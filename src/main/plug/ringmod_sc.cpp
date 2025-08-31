/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins-ringmod-sc
 * Created on: 29 авг 2025 г.
 *
 * lsp-plugins-ringmod-sc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins-ringmod-sc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins-ringmod-sc. If not, see <https://www.gnu.org/licenses/>.
 */

#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/dsp-units/units.h>
#include <lsp-plug.in/plug-fw/core/AudioBuffer.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/shared/debug.h>

#include <private/plugins/ringmod_sc.h>

namespace lsp
{
    namespace plugins
    {
        // The size of temporary buffer for audio processing
        static constexpr size_t BUFFER_SIZE     = 0x400;

        //---------------------------------------------------------------------
        // Plugin factory
        static const meta::plugin_t *plugins[] =
        {
            &meta::ringmod_sc_mono,
            &meta::ringmod_sc_stereo
        };

        static plug::Module *plugin_factory(const meta::plugin_t *meta)
        {
            return new ringmod_sc(meta);
        }

        static plug::Factory factory(plugin_factory, plugins, 2);

        //---------------------------------------------------------------------
        // Implementation
        ringmod_sc::ringmod_sc(const meta::plugin_t *meta):
            Module(meta)
        {
            // Compute the number of audio channels by the number of inputs
            nChannels           = 1;
            if (strcmp(meta->uid, meta::ringmod_sc_stereo.uid) == 0)
                nChannels           = 2;

            // Initialize other parameters
            vChannels           = NULL;
            vBuffer             = NULL;

            pBypass             = NULL;
            pGainIn             = NULL;
            pGainOut            = NULL;
            pHold               = NULL;
            pRelease            = NULL;
            pLookahead          = NULL;
            pDuck               = NULL;
            pAmount             = NULL;
            pDry                = NULL;
            pWet                = NULL;
            pDryWet             = NULL;

            sPremix.fInToSc     = GAIN_AMP_M_INF_DB;
            sPremix.fInToLink   = GAIN_AMP_M_INF_DB;
            sPremix.fLinkToIn   = GAIN_AMP_M_INF_DB;
            sPremix.fLinkToSc   = GAIN_AMP_M_INF_DB;
            sPremix.fScToIn     = GAIN_AMP_M_INF_DB;
            sPremix.fScToLink   = GAIN_AMP_M_INF_DB;

            for (size_t i=0; i<2; ++i)
            {
                sPremix.vIn[i]      = NULL;
                sPremix.vOut[i]     = NULL;
                sPremix.vSc[i]      = NULL;
                sPremix.vLink[i]    = NULL;
                sPremix.vTmpIn[i]   = NULL;
                sPremix.vTmpSc[i]   = NULL;
                sPremix.vTmpLink[i] = NULL;
            }

            sPremix.pInToSc     = NULL;
            sPremix.pInToLink   = NULL;
            sPremix.pLinkToIn   = NULL;
            sPremix.pLinkToSc   = NULL;
            sPremix.pScToIn     = NULL;
            sPremix.pScToLink   = NULL;

            pData               = NULL;
        }

        ringmod_sc::~ringmod_sc()
        {
            do_destroy();
        }

        void ringmod_sc::init(plug::IWrapper *wrapper, plug::IPort **ports)
        {
            // Call parent class for initialization
            Module::init(wrapper, ports);

            // Estimate the number of bytes to allocate
            size_t szof_channels    = align_size(sizeof(channel_t) * nChannels, OPTIMAL_ALIGN);
            size_t buf_sz           = BUFFER_SIZE * sizeof(float);
            size_t alloc            = szof_channels +
                                      buf_sz +  // vBuffer
                                      nChannels * 3 * buf_sz; // sPremix buffers

            // Allocate memory-aligned data
            uint8_t *ptr            = alloc_aligned<uint8_t>(pData, alloc, OPTIMAL_ALIGN);
            if (ptr == NULL)
                return;

            // Initialize pointers to channels and temporary buffer
            vChannels               = advance_ptr_bytes<channel_t>(ptr, szof_channels);
            vBuffer                 = advance_ptr_bytes<float>(ptr, buf_sz);

            // Initialize pre-mix
            for (size_t i=0; i<nChannels; ++i)
            {
                sPremix.vTmpIn[i]       = advance_ptr_bytes<float>(ptr, buf_sz);
                sPremix.vTmpLink[i]     = advance_ptr_bytes<float>(ptr, buf_sz);
                sPremix.vTmpSc[i]       = advance_ptr_bytes<float>(ptr, buf_sz);
            }

            for (size_t i=0; i < nChannels; ++i)
            {
                channel_t *c            = &vChannels[i];

                // Construct in-place DSP processors
                c->sBypass.construct();

                c->vIn                  = NULL;
                c->vOut                 = NULL;
                c->vSc                  = NULL;

                // Initialize fields
                c->pIn                  = NULL;
                c->pOut                 = NULL;
                c->pScIn                = NULL;
                c->pShmIn               = NULL;
            }

            // Bind ports
            lsp_trace("Binding ports");
            size_t port_id      = 0;

            // Bind input audio ports
            for (size_t i=0; i<nChannels; ++i)
                BIND_PORT(vChannels[i].pIn);

            // Bind output audio ports
            for (size_t i=0; i<nChannels; ++i)
                BIND_PORT(vChannels[i].pOut);

            // Bind sidechain audio ports
            for (size_t i=0; i<nChannels; ++i)
                BIND_PORT(vChannels[i].pScIn);

            // Bind stereo link
            SKIP_PORT("Stereo link name");
            for (size_t i=0; i<nChannels; ++i)
                BIND_PORT(vChannels[i].pShmIn);

            // Pre-mixing ports
            lsp_trace("Binding pre-mix ports");
            SKIP_PORT("Show premix overlay");
            BIND_PORT(sPremix.pInToLink);
            BIND_PORT(sPremix.pLinkToIn);
            BIND_PORT(sPremix.pLinkToSc);
            BIND_PORT(sPremix.pInToSc);
            BIND_PORT(sPremix.pScToIn);
            BIND_PORT(sPremix.pScToLink);

            // Bind common ports
            BIND_PORT(pBypass);
            BIND_PORT(pGainIn);
            BIND_PORT(pGainOut);
            BIND_PORT(pHold);
            BIND_PORT(pRelease);
            BIND_PORT(pLookahead);
            BIND_PORT(pDuck);
            BIND_PORT(pAmount);
            BIND_PORT(pDry);
            BIND_PORT(pWet);
            BIND_PORT(pDryWet);
        }

        void ringmod_sc::destroy()
        {
            Module::destroy();
            do_destroy();
        }

        void ringmod_sc::do_destroy()
        {
            // Destroy channels
            if (vChannels != NULL)
            {
                for (size_t i=0; i<nChannels; ++i)
                {
                    channel_t *c    = &vChannels[i];
                    c->sBypass.destroy();
                }
                vChannels   = NULL;
            }

            vBuffer     = NULL;

            // Free previously allocated data chunk
            if (pData != NULL)
            {
                free_aligned(pData);
                pData       = NULL;
            }
        }

        void ringmod_sc::update_sample_rate(long sr)
        {
            // Update sample rate for the bypass processors
            for (size_t i=0; i<nChannels; ++i)
            {
                channel_t *c    = &vChannels[i];
                c->sBypass.init(sr);
            }
        }

        void ringmod_sc::update_premix()
        {
            sPremix.fInToSc     = (sPremix.pInToSc != NULL)     ? sPremix.pInToSc->value()      : GAIN_AMP_M_INF_DB;
            sPremix.fInToLink   = (sPremix.pInToLink != NULL)   ? sPremix.pInToLink->value()    : GAIN_AMP_M_INF_DB;
            sPremix.fLinkToIn   = (sPremix.pLinkToIn != NULL)   ? sPremix.pLinkToIn->value()    : GAIN_AMP_M_INF_DB;
            sPremix.fLinkToSc   = (sPremix.pLinkToSc != NULL)   ? sPremix.pLinkToSc->value()    : GAIN_AMP_M_INF_DB;
            sPremix.fScToIn     = (sPremix.pScToIn != NULL)     ? sPremix.pScToIn->value()      : GAIN_AMP_M_INF_DB;
            sPremix.fScToLink   = (sPremix.pScToLink != NULL)   ? sPremix.pScToLink->value()    : GAIN_AMP_M_INF_DB;
        }

        void ringmod_sc::update_settings()
        {
            const bool bypass       = pBypass->value() >= 0.5f;
            for (size_t i=0; i<nChannels; ++i)
            {
                channel_t *c            = &vChannels[i];

                c->sBypass.set_bypass(bypass);
            }

            // Update pre-mix matrix
            update_premix();
        }

        void ringmod_sc::premix_channels(io_buffers_t * io, size_t samples)
        {
            for (size_t i=0; i<nChannels; ++i)
            {
                // Get pointers to buffers and advance position
                float * const in_buf    = sPremix.vIn[i];
                float * const out_buf   = sPremix.vOut[i];
                float * const sc_buf    = sPremix.vSc[i];
                float * const link_buf  = sPremix.vLink[i];

                io->vIn              = in_buf;
                io->vOut             = out_buf;
                io->vScIn            = sc_buf;
                io->vShmIn           = link_buf;

                // Update pointers
                sPremix.vIn[i]    = &in_buf[samples];
                sPremix.vOut[i]   = &out_buf[samples];
                if (sc_buf != NULL)
                    sPremix.vSc[i]    = &sc_buf[samples];
                if (link_buf != NULL)
                    sPremix.vLink[i]  = &link_buf[samples];

                // Perform transformation
                // (Sc, Link) -> In
                if ((sc_buf != NULL) && (sPremix.fScToIn > GAIN_AMP_M_INF_DB))
                {
                    io->vIn               = sPremix.vTmpIn[i];
                    dsp::fmadd_k4(io->vIn, in_buf, sc_buf, sPremix.fScToIn, samples);

                    if ((link_buf != NULL) && (sPremix.fLinkToIn > GAIN_AMP_M_INF_DB))
                        dsp::fmadd_k3(io->vIn, link_buf, sPremix.fLinkToIn, samples);
                }
                else if ((link_buf != NULL) && (sPremix.fLinkToIn > GAIN_AMP_M_INF_DB))
                {
                    io->vIn               = sPremix.vTmpIn[i];
                    dsp::fmadd_k4(io->vIn, in_buf, link_buf, sPremix.fLinkToIn, samples);
                }

                // (In, Link) -> Sc
                if (sPremix.fInToSc > GAIN_AMP_M_INF_DB)
                {
                    io->vScIn             = sPremix.vTmpSc[i];
                    if (sc_buf != NULL)
                        dsp::fmadd_k4(io->vScIn, sc_buf, in_buf, sPremix.fInToSc, samples);
                    else
                        dsp::mul_k3(io->vScIn, in_buf, sPremix.fInToSc, samples);

                    if ((link_buf != NULL) && (sPremix.fLinkToSc > GAIN_AMP_M_INF_DB))
                        dsp::fmadd_k3(io->vScIn, link_buf, sPremix.fLinkToSc, samples);
                }
                else if ((link_buf != NULL) && (sPremix.fLinkToSc > GAIN_AMP_M_INF_DB))
                {
                    io->vScIn             = sPremix.vTmpSc[i];
                    if (sc_buf != NULL)
                        dsp::fmadd_k4(io->vScIn, sc_buf, link_buf, sPremix.fLinkToSc, samples);
                    else
                        dsp::mul_k3(io->vScIn, link_buf, sPremix.fLinkToSc, samples);
                }

                // (In, Sc) -> Link
                if (sPremix.fInToLink > GAIN_AMP_M_INF_DB)
                {
                    io->vShmIn            = sPremix.vTmpLink[i];
                    if (link_buf != NULL)
                        dsp::fmadd_k4(io->vShmIn, link_buf, in_buf, sPremix.fInToLink, samples);
                    else
                        dsp::mul_k3(io->vShmIn, in_buf, sPremix.fInToLink, samples);

                    if ((sc_buf != NULL) && (sPremix.fScToLink > GAIN_AMP_M_INF_DB))
                        dsp::fmadd_k3(io->vShmIn, sc_buf, sPremix.fScToLink, samples);
                }
                else if ((sc_buf != NULL) && (sPremix.fScToLink > GAIN_AMP_M_INF_DB))
                {
                    io->vShmIn            = sPremix.vTmpLink[i];
                    if (link_buf != NULL)
                        dsp::fmadd_k4(io->vShmIn, link_buf, sc_buf, sPremix.fScToLink, samples);
                    else
                        dsp::mul_k3(io->vShmIn, sc_buf, sPremix.fScToLink, samples);
                }
            }
        }

        void ringmod_sc::process(size_t samples)
        {
            io_buffers_t io[2];

            // Prepare audio channels
            for (size_t i=0; i<nChannels; ++i)
            {
                channel_t *c        = &vChannels[i];

                // Initialize pointers
                sPremix.vIn[i]      = c->pIn->buffer<float>();
                sPremix.vOut[i]     = c->pOut->buffer<float>();
                sPremix.vSc[i]      = c->pScIn->buffer<float>();
                sPremix.vLink[i]    = NULL;

                core::AudioBuffer *buf = (c->pShmIn != NULL) ? c->pShmIn->buffer<core::AudioBuffer>() : NULL;
                if ((buf != NULL) && (buf->active()))
                    sPremix.vLink[i]    = buf->buffer();
            }

            // Process data
            for (size_t offset = 0; offset < samples;)
            {
                const size_t to_process     = lsp_min(samples - offset, BUFFER_SIZE);

                // Pre-mix channel data
                premix_channels(io, to_process);

                // Process each channel independently
                for (size_t i=0; i<nChannels; ++i)
                {
//                    channel_t *c            = &vChannels[i];

                    // Just pass signal to output buffer
                    dsp::copy(io->vOut, io->vIn, samples);
                }

                // Update pointer
                offset             += to_process;
            }
        }

        void ringmod_sc::dump(dspu::IStateDumper *v) const
        {
            plug::Module::dump(v);

            // It is very useful to dump plugin state for debug purposes
            v->write("nChannels", nChannels);
            v->begin_array("vChannels", vChannels, nChannels);
            for (size_t i=0; i<nChannels; ++i)
            {
                channel_t *c            = &vChannels[i];

                v->begin_object(c, sizeof(channel_t));
                {
                    v->write_object("sBypass", &c->sBypass);

                }
                v->end_object();
            }
            v->end_array();

            v->write("vBuffer", vBuffer);

            v->write("pBypass", pBypass);

            v->write("pData", pData);
        }

    } /* namespace plugins */
} /* namespace lsp */


