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

#include <lsp-plug.in/plug-fw/meta/ports.h>
#include <lsp-plug.in/shared/meta/developers.h>
#include <private/meta/ringmod_sc.h>

#define LSP_PLUGINS_RINGMOD_SC_VERSION_MAJOR       1
#define LSP_PLUGINS_RINGMOD_SC_VERSION_MINOR       0
#define LSP_PLUGINS_RINGMOD_SC_VERSION_MICRO       0

#define LSP_PLUGINS_RINGMOD_SC_VERSION  \
    LSP_MODULE_VERSION( \
        LSP_PLUGINS_RINGMOD_SC_VERSION_MAJOR, \
        LSP_PLUGINS_RINGMOD_SC_VERSION_MINOR, \
        LSP_PLUGINS_RINGMOD_SC_VERSION_MICRO  \
    )

namespace lsp
{
    namespace meta
    {
        //-------------------------------------------------------------------------
        // Plugin metadata

        // NOTE: Port identifiers should not be longer than 7 characters as it will overflow VST2 parameter name buffers
        static const port_t ringmod_sc_mono_ports[] =
        {
            // Input and output audio ports
            PORTS_MONO_PLUGIN,

            // Input controls
            BYPASS,

            PORTS_END
        };

        // NOTE: Port identifiers should not be longer than 7 characters as it will overflow VST2 parameter name buffers
        static const port_t ringmod_sc_stereo_ports[] =
        {
            // Input and output audio ports
            PORTS_STEREO_PLUGIN,

            // Input controls
            BYPASS,

            PORTS_END
        };

        static const int plugin_classes[]       = { C_UTILITY, -1 };
        static const int clap_features_mono[]   = { CF_AUDIO_EFFECT, CF_UTILITY, CF_MONO, -1 };
        static const int clap_features_stereo[] = { CF_AUDIO_EFFECT, CF_UTILITY, CF_STEREO, -1 };

        const meta::bundle_t ringmod_sc_bundle =
        {
            "ringmod_sc",
            "Ring Modulated Sidechain",
            B_UTILITIES,
            "", // TODO: provide ID of the video on YouTube
            "This plugin allows to apply a specific sidechaining technique based on ring modulation and subtraction of the original signal."
        };

        const plugin_t ringmod_sc_mono =
        {
            "Ring Modulated Sidechain Mono",
            "Ring Modulated Sidechain Mono",
            "Ring Modulated SC Mono",
            "RMSC1M",
            &developers::v_sadovnikov,
            "ringmod_sc_mono",
            {
                LSP_LV2_URI("ringmod_sc_mono"),
                LSP_LV2UI_URI("ringmod_sc_mono"),
                "rm1m",
                LSP_VST3_UID("rm1m  rmsc1m"),
                LSP_VST3UI_UID("rm1m  rmsc1m"),
                LSP_LADSPA_RINGMOD_SC_BASE + 0,
                LSP_LADSPA_URI("ringmod_sc_mono"),
                LSP_CLAP_URI("ringmod_sc_mono"),
                LSP_GST_UID("ringmod_sc_mono"),
            },
            LSP_PLUGINS_RINGMOD_SC_VERSION,
            plugin_classes,
            clap_features_mono,
            E_DUMP_STATE,
            ringmod_sc_mono_ports,
            "util/ringmod_sc.xml",
            NULL,
            mono_plugin_port_groups,
            &ringmod_sc_bundle
        };

        const plugin_t ringmod_sc_stereo =
        {
            "Ring Modulated Sidechain Stereo",
            "Ring Modulated Sidechain Stereo",
            "Ring Modulated SC Stereo",
            "RMSC1S",
            &developers::v_sadovnikov,
            "ringmod_sc_stereo",
            {
                LSP_LV2_URI("ringmod_sc_stereo"),
                LSP_LV2UI_URI("ringmod_sc_stereo"),
                "rm1s",
                LSP_VST3_UID("rm1s  rmsc1s"),
                LSP_VST3UI_UID("rm1s  rmsc1s"),
                LSP_LADSPA_RINGMOD_SC_BASE + 1,
                LSP_LADSPA_URI("ringmod_sc_stereo"),
                LSP_CLAP_URI("ringmod_sc_stereo"),
                LSP_GST_UID("ringmod_sc_stereo"),
            },
            LSP_PLUGINS_RINGMOD_SC_VERSION,
            plugin_classes,
            clap_features_stereo,
            E_DUMP_STATE,
            ringmod_sc_stereo_ports,
            "util/ringmod_sc.xml",
            NULL,
            stereo_plugin_port_groups,
            &ringmod_sc_bundle
        };
    } /* namespace meta */
} /* namespace lsp */



