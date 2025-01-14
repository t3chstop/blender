/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "BKE_spline.hh"

#include "BKE_curve_to_mesh.hh"

#include "UI_interface.h"
#include "UI_resources.h"

#include "node_geometry_util.hh"

namespace blender::nodes {

static void geo_node_curve_to_mesh_declare(NodeDeclarationBuilder &b)
{
  b.add_input<decl::Geometry>("Curve");
  b.add_input<decl::Geometry>("Profile Curve");
  b.add_output<decl::Geometry>("Mesh");
}

static void geometry_set_curve_to_mesh(GeometrySet &geometry_set,
                                       const GeometrySet &profile_set,
                                       const GeoNodeExecParams &params)
{
  if (!geometry_set.has_curve()) {
    if (!geometry_set.is_empty()) {
      params.error_message_add(NodeWarningType::Warning,
                               TIP_("No curve data available in curve input"));
    }
    return;
  }

  const CurveEval *profile_curve = profile_set.get_curve_for_read();

  if (profile_curve == nullptr) {
    Mesh *mesh = bke::curve_to_wire_mesh(*geometry_set.get_curve_for_read());
    geometry_set.replace_mesh(mesh);
  }
  else {
    Mesh *mesh = bke::curve_to_mesh_sweep(*geometry_set.get_curve_for_read(), *profile_curve);
    geometry_set.replace_mesh(mesh);
  }
}

static void geo_node_curve_to_mesh_exec(GeoNodeExecParams params)
{
  GeometrySet curve_set = params.extract_input<GeometrySet>("Curve");
  GeometrySet profile_set = params.extract_input<GeometrySet>("Profile Curve");

  if (profile_set.has_instances()) {
    params.error_message_add(NodeWarningType::Error,
                             TIP_("Instances are not supported in the profile input"));
    params.set_output("Mesh", GeometrySet());
    return;
  }

  if (!profile_set.has_curve() && !profile_set.is_empty()) {
    params.error_message_add(NodeWarningType::Warning,
                             TIP_("No curve data available in the profile input"));
  }

  curve_set.modify_geometry_sets([&](GeometrySet &geometry_set) {
    geometry_set_curve_to_mesh(geometry_set, profile_set, params);
    geometry_set.keep_only({GEO_COMPONENT_TYPE_MESH, GEO_COMPONENT_TYPE_INSTANCES});
  });

  params.set_output("Mesh", std::move(curve_set));
}

}  // namespace blender::nodes

void register_node_type_geo_curve_to_mesh()
{
  static bNodeType ntype;

  geo_node_type_base(&ntype, GEO_NODE_CURVE_TO_MESH, "Curve to Mesh", NODE_CLASS_GEOMETRY, 0);
  ntype.declare = blender::nodes::geo_node_curve_to_mesh_declare;
  ntype.geometry_node_execute = blender::nodes::geo_node_curve_to_mesh_exec;
  nodeRegisterType(&ntype);
}
