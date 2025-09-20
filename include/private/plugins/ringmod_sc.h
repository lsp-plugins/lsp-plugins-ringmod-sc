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

#ifndef PRIVATE_PLUGINS_RINGMOD_SC_H_
#define PRIVATE_PLUGINS_RINGMOD_SC_H_

#include <lsp-plug.in/dsp-units/ctl/Bypass.h>
#include <lsp-plug.in/dsp-units/util/Delay.h>
#include <lsp-plug.in/dsp-units/util/MeterGraph.h>
#include <lsp-plug.in/dsp-units/util/RingBuffer.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <private/meta/ringmod_sc.h>

namespace lsp
{
    namespace plugins
    {
        /**
         * Base class for the latency compensation delay
         */
        class ringmod_sc: public plug::Module
        {
            protected:
                enum sc_type_t
                {
                    SC_TYPE_INTERNAL,
                    SC_TYPE_EXTERNAL,
                    SC_TYPE_SHM_LINK,
                };

                enum sc_source_t
                {
                    SC_SRC_LEFT_RIGHT,
                    SC_SRC_RIGHT_LEFT,
                    SC_SRC_LEFT,
                    SC_SRC_RIGHT,
                    SC_SRC_MID_SIDE,
                    SC_SRC_SIDE_MID,
                    SC_SRC_MIDDLE,
                    SC_SRC_SIDE,
                    SC_SRC_MIN,
                    SC_SRC_MAX
                };

                enum meter_graph_t
                {
                    MG_IN,
                    MG_SC,
                    MG_GAIN,
                    MG_OUT,

                    MG_TOTAL
                };

                typedef struct io_buffers_t
                {
                    float              *vIn;
                    float              *vOut;
                    float              *vScIn;
                    float              *vShmIn;
                    float              *vMixSc;
                } io_buffers_t;

                typedef struct premix_t
                {
                    float               fInToSc;                // Input -> Sidechain mix
                    float               fInToLink;              // Input -> Link mix
                    float               fLinkToIn;              // Link -> Input mix
                    float               fLinkToSc;              // Link -> Sidechain mix
                    float               fScToIn;                // Sidechain -> Input mix
                    float               fScToLink;              // Sidechain -> Link mix

                    float              *vIn[2];                 // Input buffer
                    float              *vOut[2];                // Output buffer
                    float              *vSc[2];                 // Sidechain buffer
                    float              *vLink[2];               // Link buffer

                    float              *vTmpIn[2];              // Replacement buffer for input
                    float              *vTmpLink[2];            // Replacement buffer for link
                    float              *vTmpSc[2];              // Replacement buffer for sidechain

                    plug::IPort        *pInToSc;                // Input -> Sidechain mix
                    plug::IPort        *pInToLink;              // Input -> Link mix
                    plug::IPort        *pLinkToIn;              // Link -> Input mix
                    plug::IPort        *pLinkToSc;              // Link -> Sidechain mix
                    plug::IPort        *pScToIn;                // Sidechain -> Input mix
                    plug::IPort        *pScToLink;              // Sidechain -> Link mix
                } premix_t;

                typedef struct channel_t
                {
                    // DSP processing modules
                    dspu::Bypass        sBypass;                // Bypass
                    dspu::Delay         sInDelay;               // Input signal delay
                    dspu::Delay         sScDelay;               // Sidechain input delay
                    dspu::RingBuffer    sEnvDelay;              // Sidechain envelope delay buffer
                    dspu::MeterGraph    vGraph[MG_TOTAL];       // Meter graphs

                    float               fPeak;                  // Current sidechain peak value
                    uint32_t            nHold;                  // Hold counter
                    bool                vVisible[MG_TOTAL];     // Meter visibility
                    float               vValues[MG_TOTAL];      // Meter values
                    float              *vInData;                // Input signal data
                    float              *vBuffer;                // Temporary data

                    // Ports
                    plug::IPort        *pIn;                    // Input port
                    plug::IPort        *pOut;                   // Output port
                    plug::IPort        *pScIn;                  // Sidechain input port
                    plug::IPort        *pShmIn;                 // Shared memory link input port
                    plug::IPort        *vVisibility[MG_TOTAL];  // Meters visibility
                    plug::IPort        *vMeters[MG_TOTAL];      // Meters
                } channel_t;

            protected:
                uint32_t            nChannels;              // Number of channels
                channel_t          *vChannels;              // Processing channels
                float              *vEmptyBuffer;           // Empty buffer for audio processing
                float              *vTime;                  // Mesh time points
                float              *vBuffer;                // Temporary buffer for audio processing
                premix_t            sPremix;                // Sidechain pre-mix
                uint32_t            nType;                  // Sidechain type
                uint32_t            nSource;                // Sidechain source
                uint32_t            nLookahead;             // Lookahead
                uint32_t            nDuck;                  // Ducking
                uint32_t            nHold;                  // Hold signal
                float               fTauRelease;            // Release time constant
                float               fStereoLink;            // Stereo linking
                float               fInGain;                // Input gain
                float               fOutGain;               // Output gain
                float               fScGain;                // Sidechain gain
                float               fAmount;                // The amount of data to subtract
                float               fDry;                   // Dry amount of signal
                float               fWet;                   // Wet amount of signal
                bool                bOutIn;                 // Output inpug signal
                bool                bOutSc;                 // Output sidechain value
                bool                bActive;                // Sidechain processing is active
                bool                bInvert;                // Invert sidechain processing
                bool                bPause;                 // Pause output graph
                bool                bClear;                 // Clear output graph
                bool                bUISync;                // Synchronize mesh with UI

                plug::IPort        *pBypass;                // Bypass
                plug::IPort        *pGainIn;                // Input gain
                plug::IPort        *pGainSc;                // Sidechain gain
                plug::IPort        *pGainOut;               // Output gain
                plug::IPort        *pOutIn;                 // Output input signal
                plug::IPort        *pOutSc;                 // Output sidechain
                plug::IPort        *pActive;                // Sidechain processing is active
                plug::IPort        *pInvert;                // Invert sidechain processing
                plug::IPort        *pType;                  // Sidechain type
                plug::IPort        *pSource;                // Sidechain source
                plug::IPort        *pStereoLink;            // Stereo linking
                plug::IPort        *pHold;                  // Hold time
                plug::IPort        *pRelease;               // Release time
                plug::IPort        *pLookahead;             // Lookahead time
                plug::IPort        *pDuck;                  // Duck time
                plug::IPort        *pAmount;                // Amount
                plug::IPort        *pDry;                   // Dry gain
                plug::IPort        *pWet;                   // Wet gain
                plug::IPort        *pDryWet;                // Dry/Wet balance
                plug::IPort        *pGraphMesh;             // Meter graph mesh
                plug::IPort        *pPause;                 // Pause graph processing
                plug::IPort        *pClear;                 // Clear

                uint8_t            *pData;                  // Allocated data

            protected:
                void                do_destroy();
                void                update_premix();
                void                premix_channels(io_buffers_t *io, size_t samples);
                void                process_sidechain_type(float **sc, io_buffers_t *io_buf, size_t samples);
                void                process_sidechain_envelope(float **sc, size_t samples);
                void                process_sidechain_delays(float **sc, size_t samples);
                void                process_sidechain_stereo_link(float **sc, size_t samples);
                void                apply_sidechain_signal(io_buffers_t *io_buf, size_t samples);
                void                output_meters();
                void                output_meshes();

            public:
                explicit ringmod_sc(const meta::plugin_t *meta);
                ringmod_sc (const ringmod_sc &) = delete;
                ringmod_sc (ringmod_sc &&) = delete;
                virtual ~ringmod_sc() override;

                ringmod_sc & operator = (const ringmod_sc &) = delete;
                ringmod_sc & operator = (ringmod_sc &&) = delete;

                virtual void        init(plug::IWrapper *wrapper, plug::IPort **ports) override;
                virtual void        destroy() override;

            public:
                virtual void        update_sample_rate(long sr) override;
                virtual void        update_settings() override;
                virtual void        ui_activated() override;
                virtual void        process(size_t samples) override;
                virtual void        dump(dspu::IStateDumper *v) const override;
        };

    } /* namespace plugins */
} /* namespace lsp */


#endif /* PRIVATE_PLUGINS_RINGMOD_SC_H_ */

