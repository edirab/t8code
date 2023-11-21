/*
  This file is part of t8code.
  t8code is a C library to manage a collection (a forest) of multiple
  connected adaptive space-trees of general element classes in parallel.

  Copyright (C) 2015 the developers

  t8code is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  t8code is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with t8code; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

/** \file t8_geometry_lagrange.hxx
 * TODO: Add description
 */

#ifndef T8_GEOMETRY_LAGRANGE_HXX
#define T8_GEOMETRY_LAGRANGE_HXX

#include <string>
#include <vector>

#include <t8.h>
#include <t8_cmesh/t8_cmesh_types.h>
#include <t8_geometry/t8_geometry_with_vertices.hxx>
#include <t8_geometry/t8_geometry_with_vertices.h>

/* Abstract struct for the mapping with Lagrange basis functions */
struct t8_geometry_lagrange: public t8_geometry_with_vertices
{
 public:
  /** 
   * Constructor of the Lagrange geometry with a given dimension. The geometry
   * is compatible with all tree types and uses as many vertices as the number of Lagrange
   * basis functions used for the mapping.
   * The vertices are saved via the \ref t8_cmesh_set_tree_vertices function.
   * \param [in] dim  0 <= \a dim <= 3. Element dimension in the parametric space.
   */
  t8_geometry_lagrange (int dim);

  /* Base constructor with no arguments. We need this since it
   * is called from derived class constructors.
   * Sets dimension and name to invalid values. */
  t8_geometry_lagrange (): t8_geometry_with_vertices ()
  {
  }

  virtual ~t8_geometry_lagrange ();

  /**
   * Maps points in the reference space \f$ [0,1]^\mathrm{dim} \to \mathbb{R}^3 \f$.
   * \param [in]  cmesh       The cmesh in which the point lies.
   * \param [in]  gtreeid     The global tree (of the cmesh) in which the reference point is.
   * \param [in]  ref_coords  Array of \a dimension x \a num_coords many entries, specifying points in \f$ [0,1]^\mathrm{dim} \f$.
   * \param [in]  num_coords  Amount of points of /f$ \mathrm{dim} /f$ to map.
   * \param [out] out_coords  The mapped coordinates in physical space of \a ref_coords. The length is \a num_coords * 3.
   */
  void
  t8_geom_evaluate (t8_cmesh_t cmesh, t8_gloidx_t gtreeid, const double *ref_coords, const size_t num_coords,
                    double *out_coords) const;

  /**
   * Compute the jacobian of the \a t8_geom_evaluate map at a point in the reference space \f$ [0,1]^\mathrm{dim} \f$.
   * \param [in]  cmesh      The cmesh in which the point lies.
   * \param [in]  gtreeid    The global tree (of the cmesh) in which the reference point is.
   * \param [in]  ref_coords  Array of \a dimension x \a num_coords many entries, specifying points in \f$ [0,1]^\mathrm{dim} \f$.
   * \param [in]  num_coords  Amount of points of /f$ \mathrm{dim} /f$ to map.
   * \param [out] jacobian    The jacobian at \a ref_coords. Array of size \a num_coords x dimension x 3. Indices \f$ 3 \cdot i\f$ , \f$ 3 \cdot i+1 \f$ , \f$ 3 \cdot i+2 \f$
   *                          correspond to the \f$ i \f$-th column of the jacobian  (Entry \f$ 3 \cdot i + j \f$ is \f$ \frac{\partial f_j}{\partial x_i} \f$).
   */
  virtual void
  t8_geom_evaluate_jacobian (t8_cmesh_t cmesh, t8_gloidx_t gtreeid, const double *ref_coords, const size_t num_coords,
                             double *jacobian) const;

  /** Update a possible internal data buffer for per tree data.
   * This function is called before the first coordinates in a new tree are
   * evaluated. You can use it for example to load the polynomial degree of the 
   * Lagrange basis into an internal buffer (as is done in the Lagrange geometry).
   * \param [in]  cmesh      The cmesh.
   * \param [in]  gtreeid    The global tree.
   */
  virtual void
  t8_geom_load_tree_data (t8_cmesh_t cmesh, t8_gloidx_t gtreeid);

 private:
  const std::vector<double>
  compute_basis (const double *ref_point) const;

  void
  interpolate (const double *point, double *out_point) const;

  const std::vector<double>
  t3_basis (const double *ref_coords) const;

  const std::vector<double>
  t6_basis (const double *ref_coords) const;

  const std::vector<double>
  q4_basis (const double *ref_coords) const;

  /* Polynomial degree of the interpolation. */
  const int *degree;
};

#endif /* !T8_GEOMETRY_MAPPING_HXX! */
