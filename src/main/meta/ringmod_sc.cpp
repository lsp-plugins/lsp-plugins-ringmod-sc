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

        static const port_item_t ringmod_sc_types[] =
        {
            { "Internal",       "sidechain.internal" },
            { "External",       "sidechain.external" },
            { "Link",           "sidechain.link" },
            { NULL, NULL }
        };

        static const port_item_t ringmod_sc_sources[] =
        {
            { "Left/Right",     "sidechain.left_right"      },
            { "Right/Left",     "sidechain.right_left"      },
            { "Left",           "sidechain.left"            },
            { "Right",          "sidechain.right"           },
            { "Mid/Side",       "sidechain.mid_side"        },
            { "Side/Mid",       "sidechain.side_mid"        },
            { "Middle",         "sidechain.middle"          },
            { "Side",           "sidechain.side"            },
            { "Min",            "sidechain.min"             },
            { "Max",            "sidechain.max"             },
            { NULL, NULL }
        };

    #define RMOD_PREMIX \
        SWITCH("showpmx", "Show pre-mix overlay", "Show premix bar", 0.0f), \
        AMP_GAIN10("in2lk", "Input to Link mix", "In to Link mix", GAIN_AMP_M_INF_DB), \
        AMP_GAIN10("lk2in", "Link to Input mix", "Link to In mix", GAIN_AMP_M_INF_DB), \
        AMP_GAIN10("lk2sc", "Link to Sidechain mix", "Link to SC mix", GAIN_AMP_M_INF_DB), \
        AMP_GAIN10("in2sc", "Input to Sidechain mix", "In to SC mix", GAIN_AMP_M_INF_DB), \
        AMP_GAIN10("sc2in", "Sidechain to Input mix", "SC to In mix", GAIN_AMP_M_INF_DB), \
        AMP_GAIN10("sc2lk", "Sidechain to Link mix", "SC to Link mix", GAIN_AMP_M_INF_DB)

    #define RMOD_SHM_LINK_MONO \
        OPT_RETURN_MONO("link", "shml", "Side-chain shared memory link")

    #define RMOD_SHM_LINK_STEREO \
        OPT_RETURN_STEREO("link", "shml_", "Side-chain shared memory link")

    #define RMOD_METERS(id, name, alias) \
        SWITCH("ilv" id, "Input visiblity" name, "Show In" alias, 1), \
        METER_OUT_GAIN("ilm" id, "Input level meter" name, GAIN_AMP_P_60_DB), \
        SWITCH("slv" id, "Sidechain visiblity" name, "Show SC" alias, 1), \
        METER_OUT_GAIN("slm" id, "Sidechain level meter" name, GAIN_AMP_P_60_DB), \
        SWITCH("grv" id, "Gain reduction visiblity" name, "Show Gain" alias, 1), \
        METER_GAIN_DFL("grm" id, "Gain reduction level meter" name, GAIN_AMP_0_DB, GAIN_AMP_0_DB), \
        SWITCH("olv" id, "Output level visiblity" name, "Show Out" alias, 1), \
        METER_OUT_GAIN("olm" id, "Output level meter" name, GAIN_AMP_P_60_DB)

        static const port_t ringmod_sc_mono_ports[] =
        {
            PORTS_MONO_PLUGIN,
            PORTS_MONO_SIDECHAIN,
            RMOD_SHM_LINK_MONO,
            RMOD_PREMIX,

            BYPASS,
            IN_GAIN,
            SC_GAIN,
            OUT_GAIN,
            SWITCH("out_in", "Output input signal", "Out In", 1),
            SWITCH("out_sc", "Output sidechain signal", "Out SC", 1),

            SWITCH("active", "Sidechain processing active", "Active", 1),
            COMBO("type", "Sidechain type", "Type", 1, ringmod_sc_types),
            CONTROL("hold", "Hold time", "Hold", U_MSEC, ringmod_sc::HOLD),
            LOG_CONTROL("release", "Release time", "Release", U_MSEC, ringmod_sc::RELEASE),
            CONTROL("lk", "Lookahead time", "Lookahead", U_MSEC, ringmod_sc::LOOKAHEAD),
            CONTROL("duck", "Ducking time", "Duck", U_MSEC, ringmod_sc::DUCK),
            CONTROL("amount", "Amount", "Amount", U_DB, ringmod_sc::AMOUNT),

            SWITCH("showmx", "Show mix overlay", "Show mix bar", 0.0f),
            AMP_GAIN10("dry", "Dry gain", "Dry", GAIN_AMP_M_INF_DB),
            AMP_GAIN10("wet", "Wet gain", "Wet", GAIN_AMP_0_DB),
            PERCENTS("drywet", "Dry/Wet balance", "Dry/Wet", 100.0f, 0.1f),

            // Meters
            SWITCH("pause", "Pause graph analysis", "Pause", 0.0f),
            TRIGGER("clear", "Clear graph analysis", "Clear"),
            RMOD_METERS("", "", ""),
            MESH("mg", "Meter graphs", 1 + 1*4, ringmod_sc::TIME_MESH_SIZE + 4),

            PORTS_END
        };

        static const port_t ringmod_sc_stereo_ports[] =
        {
            PORTS_STEREO_PLUGIN,
            PORTS_STEREO_SIDECHAIN,
            RMOD_SHM_LINK_STEREO,
            RMOD_PREMIX,

            BYPASS,
            IN_GAIN,
            SC_GAIN,
            OUT_GAIN,
            SWITCH("out_in", "Output input signal", "Out In", 1),
            SWITCH("out_sc", "Output sidechain signal", "Out SC", 1),

            SWITCH("active", "Sidechain processing active", "Active", 1),
            COMBO("type", "Sidechain type", "Type", 1, ringmod_sc_types),
            COMBO("source", "Sidechain source", "Source", 0, ringmod_sc_sources),
            PERCENTS("slink", "Stereo link", "Stereo link", 0.0f, 0.1f),
            CONTROL("hold", "Hold time", "Hold", U_MSEC, ringmod_sc::HOLD),
            LOG_CONTROL("release", "Release time", "Release", U_MSEC, ringmod_sc::RELEASE),
            CONTROL("lk", "Lookahead time", "Lookahead", U_MSEC, ringmod_sc::LOOKAHEAD),
            CONTROL("duck", "Ducking time", "Duck", U_MSEC, ringmod_sc::DUCK),
            CONTROL("amount", "Amount", "Amount", U_DB, ringmod_sc::AMOUNT),

            SWITCH("showmx", "Show mix overlay", "Show mix bar", 0.0f),
            AMP_GAIN10("dry", "Dry gain", "Dry", GAIN_AMP_M_INF_DB),
            AMP_GAIN10("wet", "Wet gain", "Wet", GAIN_AMP_0_DB),
            PERCENTS("drywet", "Dry/Wet balance", "Dry/Wet", 100.0f, 0.1f),

            // Meters
            SWITCH("pause", "Pause graph analysis", "Pause", 0.0f),
            TRIGGER("clear", "Clear graph analysis", "Clear"),
            RMOD_METERS("_l", " Left", " L"),
            RMOD_METERS("_r", " Right", " R"),
            MESH("mg", "Meter graphs", 1 + 2*4, ringmod_sc::TIME_MESH_SIZE + 4),

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
            mono_plugin_sidechain_port_groups,
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
            stereo_plugin_sidechain_port_groups,
            &ringmod_sc_bundle
        };
    } /* namespace meta */
} /* namespace lsp */



