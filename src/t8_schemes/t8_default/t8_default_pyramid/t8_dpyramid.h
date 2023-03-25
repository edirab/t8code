#ifndef T8_DPYRAMID_H
#define T8_DPYRAMID_H

/** \file t8_dpyramid.h
 * TODO: document this.
 */

#include <t8.h>
#include <t8_schemes/t8_default/t8_default_tet/t8_dtet.h>

/** The number of children that a pyramid is refined into. */
#define T8_DPYRAMID_CHILDREN 10

/** The number of faces of a pyramid. */
#define T8_DPYRAMID_FACES 5

/** The number of children at a face*/
#define T8_DPYRAMID_FACE_CHILDREN 4

/** The number of corners of a pyramid */
#define T8_DPYRAMID_CORNERS 5

/** The maximum refinement level allowed for a pyramid */
/*The tetrahedral elements are linked with pyra-elements*/
#define T8_DPYRAMID_MAXLEVEL 21

/** The length of the root pyramid in integer coordinates */
#define T8_DPYRAMID_ROOT_LEN (1 << (T8_DPYRAMID_MAXLEVEL))

/** The length of a pyramid at a given level in integer coordinates */
#define T8_DPYRAMID_LEN(l) (1 << (T8_DPYRAMID_MAXLEVEL- (l)))

/** The number of types of a pyramid */
#define T8_DPYRAMID_NUM_TYPES 4

/** The type of the root pyramid*/
#define T8_DPYRAMID_ROOT_TPYE 2

/** The first type of pyramids in the shape of a pyramid*/
#define T8_DPYRAMID_FIRST_TYPE 2

/** The second type of pyramids in the shape of a pyramid*/
#define T8_DPYRAMID_SECOND_TYPE 3

/** The length of a triangle divided by the length of a pyramid.
 * This is useful to convert boundary coordinates from pyra to tri*/
#define T8_DTRI_ROOT_BY_DPYRAMID_ROOT (1 <<(T8_DTRI_MAXLEVEL - T8_DPYRAMID_MAXLEVEL))

/** The coordinates of a pyramid are integers relative to the maximum refinement. */
typedef int32_t     t8_dpyramid_coord_t;

/** The type of pyramid in 0, ...,7. The first 6 types describe tetrahedra.
 * Type 6 is an upward facing pyramid.
 * Type 7 is a downward facing pyramid.
*/
typedef int8_t t8_dpyramid_type_t;


/** This data type stores a pyramid/tetrahedron. */
typedef struct t8_dpyramid_t
{
  /** The refinement level of the element relative to the root at level 0. */
  int8_t              level;

  /** Type of the tetrahedron in 0, ...,3. */
  t8_dpyramid_type_t      type;

  t8_dpyramid_coord_t     x;        /**< The x integer coordinate of the anchor node. */
  t8_dpyramid_coord_t     y;        /**< The y integer coordinate of the anchor node. */
  t8_dpyramid_coord_t     z;        /**< The z integer coordinate of the anchor node. */
} t8_dpyramid_t;

#endif /* T8_DPYRAMID_H */
