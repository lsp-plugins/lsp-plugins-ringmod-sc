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

                typedef struct channel_t
                {
                    // DSP processing modules
                    dspu::Bypass        sBypass;            // Bypass

                    // Ports
                    plug::IPort        *pIn;                // Input port
                    plug::IPort        *pOut;               // Output port
                } channel_t;

            protected:
                uint32_t            nChannels;          // Number of channels
                channel_t          *vChannels;          // Processing channels
                float              *vBuffer;            // Temporary buffer for audio processing

                plug::IPort        *pBypass;            // Bypass

                uint8_t            *pData;              // Allocated data

            protected:
                void                do_destroy();

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

