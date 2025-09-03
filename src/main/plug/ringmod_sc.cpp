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
        static constexpr size_t BUFFER_SIZE     = 0x200;

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
            vEmptyBuffer        = NULL;
            vTime               = NULL;
            vBuffer             = NULL;

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

            nLookahead          = 0;
            nDuck               = 0;
            nHold               = 0;
            fTauRelease         = 1.0f;
            fStereoLink         = 0.0f;
            fInGain             = GAIN_AMP_0_DB;
            fAmount             = GAIN_AMP_0_DB;
            fDry                = 0.0f;
            fWet                = GAIN_AMP_0_DB;

            pBypass             = NULL;
            pGainIn             = NULL;
            pGainOut            = NULL;
            pSource             = NULL;
            nType               = SC_TYPE_EXTERNAL;
            nSource             = SC_SRC_LEFT_RIGHT;

            pType               = NULL;
            pSource             = NULL;
            pStereoLink         = NULL;
            pHold               = NULL;
            pRelease            = NULL;
            pLookahead          = NULL;
            pDuck               = NULL;
            pAmount             = NULL;
            pDry                = NULL;
            pWet                = NULL;
            pDryWet             = NULL;

            pGraphMesh          = NULL;

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
            size_t history_sz       = meta::ringmod_sc::TIME_MESH_SIZE * sizeof(float);
            size_t alloc            = szof_channels +
                                      buf_sz +  // vEmptyBuffer
                                      buf_sz +  // vBuffer
                                      history_sz + // vTime
                                      nChannels * ( // channel_t
                                          buf_sz +  // vIndata
                                          buf_sz    // vBuffer
                                      ) +
                                      nChannels * 3 * buf_sz; // sPremix buffers

            // Allocate memory-aligned data
            uint8_t *ptr            = alloc_aligned<uint8_t>(pData, alloc, OPTIMAL_ALIGN);
            if (ptr == NULL)
                return;

            // Initialize pointers to channels and temporary buffer
            vChannels               = advance_ptr_bytes<channel_t>(ptr, szof_channels);
            vEmptyBuffer            = advance_ptr_bytes<float>(ptr, buf_sz);
            vTime                   = advance_ptr_bytes<float>(ptr, history_sz);
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
                c->sInDelay.construct();
                c->sScDelay.construct();

                for (size_t j=0; j<MG_TOTAL; ++j)
                    c->vGraph[j].construct();
                c->vGraph[MG_IN].set_method(dspu::MM_ABS_MAXIMUM);
                c->vGraph[MG_SC].set_method(dspu::MM_ABS_MAXIMUM);
                c->vGraph[MG_GAIN].set_method(dspu::MM_ABS_MINIMUM);
                c->vGraph[MG_OUT].set_method(dspu::MM_ABS_MAXIMUM);

                c->fPeak                = 0.0f;
                c->nHold                = 0;
                for (size_t j=0; j<MG_TOTAL; ++j)
                {
                    c->vVisible[j]          = true;
                    c->vValues[j]           = GAIN_AMP_M_INF_DB;
                }

                c->vInData              = advance_ptr_bytes<float>(ptr, buf_sz);
                c->vBuffer              = advance_ptr_bytes<float>(ptr, buf_sz);

                // Initialize fields
                c->pIn                  = NULL;
                c->pOut                 = NULL;
                c->pScIn                = NULL;
                c->pShmIn               = NULL;

                for (size_t j=0; j<MG_TOTAL; ++j)
                {
                    c->vVisibility[j]       = NULL;
                    c->vMeters[j]           = NULL;
                }
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
            BIND_PORT(pType);
            if (nChannels > 1)
            {
                BIND_PORT(pSource);
                BIND_PORT(pStereoLink);
            }
            BIND_PORT(pHold);
            BIND_PORT(pRelease);
            BIND_PORT(pLookahead);
            BIND_PORT(pDuck);
            BIND_PORT(pAmount);

            SKIP_PORT("Show dry/wet overlay");
            BIND_PORT(pDry);
            BIND_PORT(pWet);
            BIND_PORT(pDryWet);

            // Bind meters
            lsp_trace("Binding meters");
            for (size_t i=0; i<nChannels; ++i)
                for (size_t j=0; j<MG_TOTAL; ++j)
                {
                    BIND_PORT(vChannels[i].vVisibility[j]);
                    BIND_PORT(vChannels[i].vMeters[j]);
                }

            BIND_PORT(pGraphMesh);

            // Initialize buffers
            dsp::fill_zero(vEmptyBuffer, BUFFER_SIZE);

            float delta = meta::ringmod_sc::TIME_HISTORY_MAX / (meta::ringmod_sc::TIME_MESH_SIZE - 1);
            for (size_t i=0; i<meta::ringmod_sc::TIME_MESH_SIZE; ++i)
                vTime[i]    = meta::ringmod_sc::TIME_HISTORY_MAX - i*delta;
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
                    c->sInDelay.destroy();
                    c->sScDelay.destroy();

                    for (size_t j=0; j<MG_TOTAL; ++j)
                        c->vGraph[j].destroy();
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
            const size_t samples_per_dot    = dspu::seconds_to_samples(sr, meta::ringmod_sc::TIME_HISTORY_MAX / meta::ringmod_sc::TIME_MESH_SIZE);
            const size_t in_max_delay = dspu::millis_to_samples(sr, meta::ringmod_sc::LOOKAHEAD_MAX);
            const size_t sc_max_delay =
                in_max_delay +
                dspu::millis_to_samples(sr, meta::ringmod_sc::DUCK_MAX) +
                BUFFER_SIZE;

            // Update sample rate for the bypass processors
            for (size_t i=0; i<nChannels; ++i)
            {
                channel_t *c    = &vChannels[i];
                c->sBypass.init(sr);
                c->sInDelay.init(in_max_delay + BUFFER_SIZE);
                c->sScDelay.init(sc_max_delay + BUFFER_SIZE);

                for (size_t j=0; j<MG_TOTAL; ++j)
                    c->vGraph[j].set_period(samples_per_dot);
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

            // Update pre-mix matrix
            update_premix();

            // Update sidechain processing
            nType                   = pType->value();
            nSource                 = (pSource != NULL) ? pSource->value() : SC_SRC_LEFT_RIGHT;
            fStereoLink             = (pStereoLink != NULL) ? lsp_max(pStereoLink->value(), 0.0f) : 0.0f;
            nHold                   = dspu::millis_to_samples(fSampleRate, pHold->value());
            const float release     = pRelease->value();
            fTauRelease             = 1.0f - expf(logf(1.0f - M_SQRT1_2) / (dspu::millis_to_samples(fSampleRate, release)));
            nLookahead              = dspu::millis_to_samples(fSampleRate, pLookahead->value());
            nDuck                   = nLookahead + dspu::millis_to_samples(fSampleRate, pDuck->value());
            fAmount                 = dspu::db_to_gain(pAmount->value());

            for (size_t i=0; i<nChannels; ++i)
            {
                channel_t *c            = &vChannels[i];

                c->sBypass.set_bypass(bypass);
                c->sInDelay.set_delay(nLookahead);

                for (size_t j=0; j<MG_TOTAL; ++j)
                    c->vVisible[j]          = c->vVisibility[j]->value() >= 0.5f;
            }

            // Compute Dry/Wet balance
            const float out_gain    = pGainOut->value();
            const float dry_gain    = pDry->value();
            const float wet_gain    = pWet->value();
            const float drywet      = pDryWet->value() * 0.01f;

            fInGain                 = pGainIn->value();
            fDry                    = (dry_gain * drywet + 1.0f - drywet) * out_gain;
            fWet                    = wet_gain * drywet * out_gain;

            // Report latency
            set_latency(nLookahead);
        }

        void ringmod_sc::premix_channels(io_buffers_t * io_buf, size_t samples)
        {
            for (size_t i=0; i<nChannels; ++i)
            {
                io_buffers_t * const io = &io_buf[i];

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

        void ringmod_sc::process_sidechain_type(float **sc, io_buffers_t * io, size_t samples)
        {
            // Select the source for the specific type of sidechain
            for (size_t i=0; i<nChannels; ++i)
            {
                float *buf          = (nType == SC_TYPE_EXTERNAL) ? io->vScIn :
                                      (nType == SC_TYPE_SHM_LINK) ? io->vShmIn :
                                      io->vIn;

                sc[i]               = (buf != NULL) ? buf : vEmptyBuffer;
            }

            // Apply sidechain pre-processing depending on selected source (stereo only)
            if (nChannels <= 1)
                return;

            switch (nSource)
            {
                case SC_SRC_RIGHT_LEFT:
                    lsp::swap(sc[0], sc[1]);
                    break;

                case SC_SRC_LEFT:
                    sc[1]   = sc[0];
                    break;

                case SC_SRC_RIGHT:
                    sc[0]   = sc[1];
                    break;

                case SC_SRC_MID_SIDE:
                    dsp::lr_to_ms(vChannels[0].vBuffer, vChannels[1].vBuffer, sc[0], sc[1], samples);
                    sc[0]   = vChannels[0].vBuffer;
                    sc[1]   = vChannels[1].vBuffer;
                    break;

                case SC_SRC_SIDE_MID:
                    dsp::lr_to_ms(vChannels[1].vBuffer, vChannels[0].vBuffer, sc[0], sc[1], samples);
                    sc[0]   = vChannels[0].vBuffer;
                    sc[1]   = vChannels[1].vBuffer;
                    break;

                case SC_SRC_MIDDLE:
                    dsp::lr_to_mid(vBuffer, sc[0], sc[1], samples);
                    sc[0]   = vBuffer;
                    sc[1]   = vBuffer;
                    break;

                case SC_SRC_SIDE:
                    dsp::lr_to_side(vBuffer, sc[0], sc[1], samples);
                    sc[0]   = vBuffer;
                    sc[1]   = vBuffer;
                    break;

                case SC_SRC_MIN:
                    dsp::pamin3(vBuffer, sc[0], sc[1], samples);
                    sc[0]   = vBuffer;
                    sc[1]   = vBuffer;
                    break;

                case SC_SRC_MAX:
                    dsp::pamax3(vBuffer, sc[0], sc[1], samples);
                    sc[0]   = vBuffer;
                    sc[1]   = vBuffer;
                    break;

                case SC_SRC_LEFT_RIGHT: // already properly mapped
                default:
                    break;
            }
        }

        void ringmod_sc::process_sidechain_envelope(float **sc, size_t samples)
        {
            // Pre-process sidechain data for each channel
            for (size_t i=0; i<nChannels; ++i)
            {
                channel_t *c        = &vChannels[i];

                // Transform sidechain signal into envelope
                uint32_t hold       = c->nHold;
                float peak          = c->fPeak;
                const float *src    = sc[i];
                float * const dst   = c->vBuffer;

                for (size_t j=0; j<samples; ++j)
                {
                    float s             = fabsf(src[j]);      // Rectify input
                    if (peak > s)
                    {
                        // Current rectified sample is below the peak value
                        if (hold > 0)
                        {
                            s                   = peak;             // Hold peak value
                            --hold;
                        }
                        else
                        {
                            s                  += (s - peak) * fTauRelease;
                            peak                = s;
                        }
                    }
                    else
                    {
                        peak                = s;
                        hold                = nHold;                // Reset hold counter
                    }
                    dst[j]              = s;
                }

                // Update parameters
                c->nHold            = hold;
                c->fPeak            = peak;
                sc[i]               = dst;
            }
        }

        void ringmod_sc::process_sidechain_delays(float **sc, size_t samples)
        {
            for (size_t i=0; i<nChannels; ++i)
            {
                channel_t *c        = &vChannels[i];

                // Now push the buffer contents to the ring buffer
                c->sScDelay.append(c->vBuffer, samples);

                // Apply lookahead and ducking
                if (nLookahead > 0)
                {
                    c->sScDelay.get(vBuffer, nLookahead + samples, samples);
                    dsp::pmax2(c->vBuffer, vBuffer, samples);
                }
                if (nDuck > nLookahead)
                {
                    c->sScDelay.get(vBuffer, nDuck + samples, samples);
                    dsp::pmax2(c->vBuffer, vBuffer, samples);
                }
            }
        }

        void ringmod_sc::process_sidechain_stereo_link(float **sc, size_t samples)
        {
            const float slink   = fStereoLink;
            if (slink <= 0.0f)
                return;

            float *lbuf         = sc[0];
            float *rbuf         = sc[1];

            // For both channels: find the minimum one and try to raise to maximum one
            // proportionally to the stereo link setup
            for (size_t i=0; i<samples; ++i)
            {
                const float ls      = lbuf[i];
                const float rs      = rbuf[i];
                if (ls < rs)
                    lbuf[i]             = ls + (rs - ls) * slink;
                else
                    rbuf[i]             = rs + (ls - rs) * slink;
            }
        }

        void ringmod_sc::process_sidechain_signal(io_buffers_t *io, size_t samples)
        {
            float *sc[2];

            // Process sidechain signal depending on selected sidechain type
            process_sidechain_type(sc, io, samples);

            // Transform sidechain into envelope
            process_sidechain_envelope(sc, samples);

            // Apply lookahead and ducking
            process_sidechain_delays(sc, samples);

            // Now we can perform linking
            if (nChannels > 1)
                process_sidechain_stereo_link(sc, samples);
        }

        void ringmod_sc::apply_sidechain_signal(io_buffers_t *io_buf, size_t samples)
        {
            // Process each channel independently
            for (size_t i=0; i<nChannels; ++i)
            {
                channel_t * const c     = &vChannels[i];
                io_buffers_t * const io = &io_buf[i];

                // Apply lookahead delay
                c->sInDelay.process(c->vInData, io->vIn, fInGain, samples);
                c->vGraph[MG_IN].process(c->vInData, samples);
                c->vValues[MG_IN]   = lsp_max(c->vValues[MG_IN], dsp::abs_max(c->vInData, samples));
                c->vGraph[MG_SC].process(c->vBuffer, samples);
                c->vValues[MG_SC]   = lsp_max(c->vValues[MG_SC], dsp::abs_max(c->vBuffer, samples));

                // Modulate the signal with the sidechain and subtract from original signal
                dsp::mul2(c->vBuffer, c->vInData, samples);                         // c->vBuffer = ring-modulated data
                dsp::fmsub_k4(vBuffer, c->vInData, c->vBuffer, fAmount, samples);   // vBuffer    = processed signal

                for (size_t j=0; j<samples; ++j)
                    c->vBuffer[j]       = lsp_max(0.0f, GAIN_AMP_0_DB - c->vBuffer[j] * fAmount); // c->vBuffer = gain reduction
                c->vGraph[MG_GAIN].process(c->vBuffer, samples);
                c->vValues[MG_GAIN] = lsp_min(c->vValues[MG_GAIN], dsp::abs_min(c->vBuffer, samples));

                // Apply Dry/wet balance
                dsp::mix2(vBuffer, c->vInData, fWet, fDry, samples);

                c->vGraph[MG_OUT].process(c->vBuffer, samples);
                c->vValues[MG_OUT]  = lsp_max(c->vValues[MG_OUT], dsp::abs_max(vBuffer, samples));

                // Apply bypass
                c->sBypass.process(io->vOut, c->vInData, vBuffer, samples);
            }
        }

        void ringmod_sc::output_meters()
        {
            for (size_t i=0; i<nChannels; ++i)
            {
                channel_t * const c     = &vChannels[i];
                for (size_t j=0; j<MG_TOTAL; ++j)
                    c->vMeters[j]->set_value(c->vValues[j]);
            }
        }

        void ringmod_sc::output_meshes()
        {
            plug::mesh_t *mesh = (pGraphMesh != NULL) ? pGraphMesh->buffer<plug::mesh_t>() : NULL;
            if ((mesh != NULL) && (!mesh->isEmpty()))
            {
                size_t index    = 0;
                float *v        = mesh->pvData[index++];

                // Time
                dsp::copy(&v[2], vTime, meta::ringmod_sc::TIME_MESH_SIZE);
                v[0]            = v[2] + 0.5f;
                v[1]            = v[0];
                v              += meta::ringmod_sc::TIME_MESH_SIZE + 2;
                v[0]            = v[-1] - 0.5f;
                v[1]            = v[-1];

                // Channels
                for (size_t i=0; i<nChannels; ++i)
                {
                    channel_t * const c     = &vChannels[i];

                    for (size_t j=0; j<MG_TOTAL; ++j)
                    {
                        const float g   = (j == MG_GAIN) ? GAIN_AMP_0_DB : GAIN_AMP_M_INF_DB;

                        v               = mesh->pvData[index++];
                        dsp::copy(&v[2], c->vGraph[j].data(), meta::ringmod_sc::TIME_MESH_SIZE);

                        v[0]            = g;
                        v[1]            = v[2];
                        v              += meta::ringmod_sc::TIME_MESH_SIZE + 2;
                        v[0]            = v[-1];
                        v[1]            = g;
                    }
                }

                // Update mesh state
                mesh->data(index, meta::ringmod_sc::TIME_MESH_SIZE + 4);
            }
        }

        void ringmod_sc::process(size_t samples)
        {
            io_buffers_t io_buf[2];

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

                // Initialize meters
                c->vValues[MG_IN]   = GAIN_AMP_M_INF_DB;
                c->vValues[MG_SC]   = GAIN_AMP_M_INF_DB;
                c->vValues[MG_GAIN] = GAIN_AMP_0_DB;
                c->vValues[MG_OUT]  = GAIN_AMP_M_INF_DB;
            }

            // Process data
            for (size_t offset = 0; offset < samples;)
            {
                const size_t to_process     = lsp_min(samples - offset, BUFFER_SIZE);

                // Do processing
                premix_channels(io_buf, to_process);
                process_sidechain_signal(io_buf, to_process);
                apply_sidechain_signal(io_buf, to_process);

                // Update pointer
                offset             += to_process;
            }

            // Output meters and meshes
            output_meters();
            output_meshes();
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


