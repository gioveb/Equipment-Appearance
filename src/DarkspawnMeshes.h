// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <array>
#include <string_view>
namespace weapon_scale {
// Exact basenames verified against vanilla variation labels AND mesh inventory.
// Replacements retaining these resource names keep the same item category.
inline constexpr std::array<std::string_view,15> DarkspawnMeshes{
 "w_axe_dsaxa_0.msh",
 "w_cbw_dscba_0.msh",
 "w_dgr_dsdga_0.msh",
 "w_gax_dsgaa_0.msh",
 "w_gsw_dsgsa_0.msh",
 "w_lbw_dslba_0.msh",
 "w_lsw_dssa_0.msh",
 "w_mal_dsmla_0.msh",
 "w_mce_dsmca_0.msh",
 "w_qui_db01a_0.msh",
 "w_qui_dq01a_0.msh",
 "w_sbw_dssba_0.msh",
 "w_shd_dsla_0.msh",
 "w_shd_dssa_0.msh",
 "w_stf_dssta_0.msh",
 };
}
