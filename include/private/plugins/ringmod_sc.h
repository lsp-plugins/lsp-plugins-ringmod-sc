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

#include <lsp-plug.in/dsp-units/util/Delay.h>
#include <lsp-plug.in/dsp-units/ctl/Bypass.h>
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
                enum mode_t
                {
                    CD_MONO,
                    CD_STEREO,
                    CD_X2_STEREO
                };

                typedef struct io_buffers_t
                {
                    float              *vIn;
                    float              *vOut;
                    float              *vScIn;
                    float              *vShmIn;
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

                    float              *vIn;                    // Input data
                    float              *vOut;                   // Output data
                    float              *vSc;                    // Sidechain data

                    // Ports
                    plug::IPort        *pIn;                    // Input port
                    plug::IPort        *pOut;                   // Output port
                    plug::IPort        *pScIn;                  // Sidechain input port
                    plug::IPort        *pShmIn;                 // Shared memory link input port
                } channel_t;

            protected:
                uint32_t            nChannels;              // Number of channels
                channel_t          *vChannels;              // Processing channels
                float              *vBuffer;                // Temporary buffer for audio processing
                premix_t            sPremix;                // Sidechain pre-mix

                plug::IPort        *pBypass;                // Bypass
                plug::IPort        *pGainIn;                // Input gain
                plug::IPort        *pGainOut;               // Output gain
                plug::IPort        *pHold;                  // Hold time
                plug::IPort        *pRelease;               // Release time
                plug::IPort        *pLookahead;             // Lookahead time
                plug::IPort        *pDuck;                  // Duck time
                plug::IPort        *pAmount;                // Amount
                plug::IPort        *pDry;                   // Dry gain
                plug::IPort        *pWet;                   // Wet gain
                plug::IPort        *pDryWet;                // Dry/Wet balance

                uint8_t            *pData;                  // Allocated data

            protected:
                void                do_destroy();
                void                update_premix();
                void                premix_channels(io_buffers_t * io, size_t samples);

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
                virtual void        process(size_t samples) override;
                virtual void        dump(dspu::IStateDumper *v) const override;
        };

    } /* namespace plugins */
} /* namespace lsp */


#endif /* PRIVATE_PLUGINS_RINGMOD_SC_H_ */

