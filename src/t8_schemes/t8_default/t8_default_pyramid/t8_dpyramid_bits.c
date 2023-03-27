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

#include "t8_dpyramid_bits.h"
#include "t8_dpyramid_connectivity.h"
#include <sc_functions.h>
#include <p4est_bits.h>
#include <t8_schemes/t8_default/t8_default_tet/t8_dtet_bits.h>
#include <t8_schemes/t8_default/t8_default_tet/t8_dtet_connectivity.h>
#include <t8_schemes/t8_default/t8_default_tri/t8_dtri_bits.h>

typedef int8_t t8_dpyramid_cube_id_t;

/*********GETTER******/
int
t8_dpyramid_get_level (const t8_dpyramid_t *p)
{
  T8_ASSERT (0 <= p->level
             && p->level <= T8_DPYRAMID_MAXLEVEL);
  return p->level;
}

t8_dpyramid_type_t
t8_dpyramid_get_type (const t8_dpyramid_t *p)
{
  return p->type;
}

t8_element_shape_t
t8_dpyramid_shape (const t8_dpyramid_t *p)
{
  T8_ASSERT (0 <= p->level
             && p->level <= T8_DPYRAMID_MAXLEVEL);
  /*The pyramid has the shape of a tetrahedron */
  if (t8_dpyramid_get_type(p) == T8_DPYRAMID_FIRST_PYRA_TYPE ||t8_dpyramid_get_type(p) == T8_DPYRAMID_SECOND_PYRA_TYPE) {
    return T8_ECLASS_PYRAMID;
  }
  else {
    return T8_ECLASS_TET;
  }
}

int
t8_dpyramid_num_corners (const t8_dpyramid_t *p)
{
  T8_ASSERT (0 <= p->level
             && p->level <= T8_DPYRAMID_MAXLEVEL);
  if (t8_dpyramid_shape (p) == T8_ECLASS_TET) {
    return T8_DPYRAMID_TET_CORNERS;
  }
  else {
    return T8_DPYRAMID_PYRA_CORNERS;
  }
}

int
t8_dpyramid_num_children (const t8_dpyramid_t *p)
{
  T8_ASSERT (0 <= p->level
             && p->level <= T8_DPYRAMID_MAXLEVEL);
  if (t8_dpyramid_shape (p) == T8_ECLASS_TET) {
    return T8_DPYRAMID_TET_CHILDREN;
  }
  else {
    return T8_DPYRAMID_PYRA_CHILDREN;
  }
}

int
t8_dpyramid_num_siblings (const t8_dpyramid_t *p)
{
  T8_ASSERT (0 <= p->level
             && p->level <= T8_DPYRAMID_MAXLEVEL);
  t8_dpyramid_t parent;
  t8_dpyramid_parent(p, &parent);
  return t8_dpyramid_num_children(&parent);
}

int
t8_dpyramid_num_faces (const t8_dpyramid_t *p)
{
  T8_ASSERT (0 <= p->level
             && p->level <= T8_DPYRAMID_MAXLEVEL);
  if (t8_dpyramid_shape (p) == T8_ECLASS_TET) {
    return T8_DPYRAMID_TET_FACES;
  }
  else {
    return T8_DPYRAMID_PYRA_FACES;
  }
}

int
t8_dpyramid_max_num_faces (const t8_dpyramid_t *p)
{
  T8_ASSERT (0 <= p->level
             && p->level <= T8_DPYRAMID_MAXLEVEL);
  return T8_DPYRAMID_MAX_FACES;
}


/**Cube helper**/

static t8_dpyramid_cube_id_t
t8_dpyramid_compute_cubeid (const t8_dpyramid_t *p, const int level)
{
  t8_dpyramid_cube_id_t cube_id = 0;

  T8_ASSERT (0 <= p->level
             && p->level <= T8_DPYRAMID_MAXLEVEL);
  const t8_dpyramid_coord_t h = T8_DPYRAMID_LEN (level);

  if (level == 0) {
    return 0;
  }
  for(int i=0; i<T8_DPYRAMID_DIM; i++){
    cube_id |= ((p->coords[i] & h) ? 1<<i : 0);
  }
  return cube_id;
}

int8_t t8_dpyramid_compute_type_at_level(const t8_dpyramid_t *p, int level){
  int8_t type = 0;
  t8_dpyramid_type_t type_at_levels = 0;
  T8_ASSERT (0 <= p->level
             && p->level <= T8_DPYRAMID_MAXLEVEL);
  const t8_dpyramid_coord_t h = T8_DPYRAMID_LEN (level);

  for(int e = 0; e < T8_DPYRAMID_NUM_EQUATIONS; e++)
  {
    t8_dpyramid_coord_t coord_v0 = p->coords[t8_dpyramid_type_edge_equations[e][0]];
    t8_dpyramid_coord_t coord_v1 = p->coords[t8_dpyramid_type_edge_equations[e][1]];
    t8_dpyramid_coord_t type_at_levels = (~coord_v0 & coord_v1);
    if (p->type & (1<<e)){
      type_at_levels |= ((coord_v0 & coord_v1) | (~coord_v0 & ~coord_v1));
    }
    type |= ((type_at_levels & h) ? 1<<e : 0);
  }
  return type;  
}


/**
 * Set the \a shift last bits of every coordinate to zero. 
 * 
 * \param[in, out]  p     Input pyramid
 * \param[in]       shift Number of bits to set to zero
 */
static void
t8_dpyramid_cut_coordinates (t8_dpyramid_t *p, const int shift)
{
  T8_ASSERT (0 <= shift && shift <= T8_DPYRAMID_MAXLEVEL);
  for (int i=0; i<T8_DPYRAMID_DIM; i++){
    p->coords[i] = (p->coords[i] >> shift) << shift;
  }
}

/*Copies a pyramid from source to dest*/
void
t8_dpyramid_copy (const t8_dpyramid_t *source, t8_dpyramid_t *dest)
{
  T8_ASSERT (source != NULL && dest != NULL);
  if (source == dest) {
    return;
  }
  memcpy (dest, source, sizeof (t8_dpyramid_t));
}

int
t8_dpyramid_compare (const t8_dpyramid_t *p1, const t8_dpyramid_t *p2)
{
  const int           maxlvl = SC_MAX (p1->level, p2->level);

  const t8_linearidx_t id1 = t8_dpyramid_linear_id (p1, maxlvl);
  const t8_linearidx_t id2 = t8_dpyramid_linear_id (p2, maxlvl);
  if (id1 == id2) {
    /* The linear ids are the same, the pyramid with the smaller level
     * is considered smaller */
    if (p1->level == p2->level) {
      T8_ASSERT (p1->type == p2->type);
      return 0;
    }
    else {
      return p1->level - p2->level;
    }
  }
  /* return negative if id1 < id2, zero if id1 = id2, positive if id1 >
     id2 */
  return id1 < id2 ? -1 : 1;
}

int
t8_dpyramid_is_valid (const t8_dpyramid_t *p)
{
  int                 is_valid;
  const t8_dpyramid_coord_t max_coord =
    ((int64_t) 2 * T8_DPYRAMID_ROOT_LEN) - 1;
  const t8_element_shape_t shape = t8_dpyramid_shape (p);
  /*Check the level */
  is_valid = 0 <= p->level
    && p->level <= T8_DPYRAMID_MAXLEVEL;
  /*Check coordinates, we allow a boundary layer around the root-pyramid */
  for (int i=0; i<T8_DPYRAMID_DIM; i++){
    is_valid = is_valid && -T8_DPYRAMID_ROOT_LEN <= p->coords[i]
      && p->coords[i] <= max_coord;
  }

  /*The shape can be a pyramid or a tet */
  is_valid = is_valid && (shape == T8_ECLASS_PYRAMID
                          || shape == T8_ECLASS_TET);
  /*Check the type */
  is_valid = is_valid && 0 <= t8_dpyramid_get_type(p)
    && t8_dpyramid_get_type(p) < T8_DPYRAMID_NUM_TYPES;

  return is_valid;
}

void
t8_dpyramid_debug_print (const t8_dpyramid_t *p)
{
  t8_debugf
    ("x: %i, y: %i, z: %i, type %i, level: %i\n",
     p->coords[0], p->coords[1], p->coords[2], t8_dpyramid_get_type(p), p->level);
}


static void t8_dpyramid_root(t8_dpyramid_t *p)
{
  p->level=0;
  p->type=T8_DPYRAMID_ROOT_TPYE;
  p->coords[0] = p->coords[1] = p->coords[2] = 0;
}

/**SFC functionality*/
int
t8_dpyramid_child_id (const t8_dpyramid_t *p)
{
  T8_ASSERT (p->level >= 0);
  if (p->level == 0) {
    return -1;
  }
  const t8_dpyramid_cube_id_t cube_id =
    t8_dpyramid_compute_cubeid (p, p->level);
  T8_ASSERT (t8_dpyramid_type_cubeid_to_Iloc[t8_dpyramid_get_type(p)][cube_id] >= 0);
  return t8_dpyramid_type_cubeid_to_Iloc[t8_dpyramid_get_type(p)][cube_id];
}

void
t8_dpyramid_child (const t8_dpyramid_t *elem, const int child_id,
                   t8_dpyramid_t *child)
{
  T8_ASSERT (0 <= child_id && child_id < T8_DPYRAMID_CHILDREN);
  T8_ASSERT (0 <= elem->level
             && elem->level <= T8_DPYRAMID_MAXLEVEL);
  /* Compute the cube id and shift the coordinates accordingly */
  const t8_dpyramid_cube_id_t cube_id =
    t8_dpyramid_type_Iloc_to_childcubeid[elem->type][child_id];
  T8_ASSERT (cube_id >= 0);
  const t8_dpyramid_coord_t length =
    T8_DPYRAMID_LEN (elem->level + 1);

  for (int i=0; i<T8_DPYRAMID_DIM; i++){
    child->coords[i] = elem->coords[i] + ((cube_id & 1<<i) ? length : 0);
  }
  child->level = elem->level + 1;
  child->type =
    t8_dpyramid_type_Iloc_to_childtype[elem->type][child_id];

  T8_ASSERT (child->type >= 0);
}

int
t8_dpyramid_is_family (t8_dpyramid_t **fam)
{
  t8_dpyramid_t parent, compare;
  /* Take the parent of the first element as baseline to compare against */
  t8_dpyramid_parent(fam[0],&parent);
  const int num_children = t8_dpyramid_num_children(&parent);
  for(int childid=0; childid < num_children; childid++){
    /* check whether each element has the same parent */
    t8_dpyramid_parent(fam[childid], &compare);
    if (t8_dpyramid_compare(&parent, &compare)) return 0;
    /* check whether each element is the correct child of the collective parent */
    /* Could be replaced by type comparison as level is already checked in parent comparison */
    t8_dpyramid_child(&parent, childid, &compare);
    if (t8_dpyramid_compare(fam[childid], &compare)) return 0;
  }
  return 1;
}

int
t8_dpyramid_is_inside_root (const t8_dpyramid_t *p)
{
  return t8_dpyramid_compute_type_at_level (p, 0) == T8_DPYRAMID_ROOT_TPYE;
}

void
t8_dpyramid_first_descendant (const t8_dpyramid_t *p, t8_dpyramid_t *desc,
                              const int level)
{
  T8_ASSERT (level >= p->level);
  T8_ASSERT (0 <= level && level <= T8_DPYRAMID_MAXLEVEL);
  /*The first descendant of a pyramid has the same anchor coords and type, but another level */
  t8_dpyramid_copy (p, desc);
  desc->level = level;
}

void
t8_dpyramid_last_descendant (const t8_dpyramid_t *p, t8_dpyramid_t *desc,
                             const int level)
{
  T8_ASSERT (level >= p->level);
  t8_dpyramid_copy (p, desc);
  desc->level = level;
  /* Shift the coords to the eights cube. The type of the last descendant is
  * is the type of the input element*/
  t8_dpyramid_coord_t coord_offset =
    T8_DPYRAMID_LEN (p->level) - T8_DPYRAMID_LEN (level);
  for (int i=0; i < T8_DPYRAMID_DIM; i++){
    desc->coords[i] |= coord_offset;
  }
}

int
t8_dpyramid_ancestor_id (const t8_dpyramid_t *p, const int level)
{
  t8_dpyramid_t       ancestor;
  T8_ASSERT (0 <= p->level
             && p->level <= T8_DPYRAMID_MAXLEVEL);
  t8_dpyramid_ancestor (p, level, &ancestor);
  return t8_dpyramid_child_id (&ancestor);
}


void
t8_dpyramid_children (const t8_dpyramid_t *p, t8_dpyramid_t **c)
{
  T8_ASSERT (0 <= p->level
             && p->level <= T8_DPYRAMID_MAXLEVEL);
  const int           num_children = t8_dpyramid_num_children (p);
  for (int i = num_children - 1; i >= 0; i--) {
    t8_dpyramid_child (p, i, c[i]);
  }
}

void
t8_dpyramid_parent (const t8_dpyramid_t *p, t8_dpyramid_t *parent)
{
  T8_ASSERT (p->level > 0);
  const t8_dpyramid_coord_t length = T8_DPYRAMID_LEN (p->level);
  const t8_dpyramid_cube_id_t cube_id = t8_dpyramid_compute_cubeid (p, p->level);

  for(int i=0; i<T8_DPYRAMID_DIM; i++) {
    parent->coords[i] = p->coords[i] & ~length;
  }
  parent->type = t8_dpyramid_type_cubeid_to_parenttype[p->type][cube_id];
  parent->level = p->level - 1;

  T8_ASSERT (parent->level >= 0);
}

void
t8_dpyramid_successor (const t8_dpyramid_t *elem,
                                 t8_dpyramid_t *succ, const int level)
{
  T8_ASSERT (1 <= level && level <= T8_DPYRAMID_MAXLEVEL);
  t8_dpyramid_copy (elem, succ);
  succ->level = level;
  T8_ASSERT (succ->type >= 0);
  const int           child_id = t8_dpyramid_child_id (elem);
  /*Compute number of children */
  const int           num_siblings = t8_dpyramid_num_siblings (elem);
  T8_ASSERT (0 <= child_id && child_id < num_siblings);
  if (child_id == num_siblings - 1) {
    /* Last-child-case. The successor is the successor of the parent element,
     * but with the given level */
    t8_dpyramid_successor (succ, succ, level - 1);
    succ->level = level;
    /* set bits from elem-->level to level to 0, emulates first descendant */
    int                 shift = T8_DPYRAMID_MAXLEVEL - level + 1;
    t8_dpyramid_cut_coordinates (succ, shift);
  }
  else {
    /* Not the last element. Compute child with local ID child_id+1 */
    t8_dpyramid_parent (succ, succ);
    t8_dpyramid_child (succ, child_id + 1, succ);
  }
}

void
t8_dpyramid_ancestor (const t8_dpyramid_t *pyra, const int level,
                      t8_dpyramid_t *anc)
{
  T8_ASSERT (0 <= level && level <= pyra->level);
  t8_dpyramid_copy (pyra, anc);
  if (pyra->level == level) {
    return;
  }
  else if (level == pyra->level - 1) {
    /* We can reuse the parent computation if we want to go only one level up. */
    t8_dpyramid_parent (pyra, anc);
    return;
  }
  /* The coordinates and the type of the anc are defined by the level. */
  t8_dpyramid_cut_coordinates (anc, T8_DPYRAMID_MAXLEVEL - level);
  anc->type = t8_dpyramid_compute_type_at_level(pyra, level);
  anc->level = level;
}


static int
t8_dpyramid_nca_level(const t8_dpyramid_t *pyra1,
                      const t8_dpyramid_t *pyra2)
{
  SC_ABORT("not implemented\n");
  return 0;
}

void
t8_dpyramid_nearest_common_ancestor (const t8_dpyramid_t *pyra1,
                                     const t8_dpyramid_t *pyra2,
                                     t8_dpyramid_t *nca)
{
  const int8_t nca_level = t8_dpyramid_nca_level(pyra1,pyra2);
  t8_dpyramid_ancestor(pyra1, nca_level, nca);
}



/*******Linear id stuff *************/
static t8_linearidx_t t8_dpyramid_num_descendants_at_leveldiff(const t8_dpyramid_t *p, const int leveldiff)
{
  t8_linearidx_t two_to_l = 1>>leveldiff;
  t8_linearidx_t eight_to_l = 8>>leveldiff;
  if(t8_dpyramid_shape(p) == T8_ECLASS_PYRAMID){
    return (eight_to_l >> 2 - two_to_l)/3;
  }else{
    return (eight_to_l >> 1 + two_to_l)/3;
  }  
}

static t8_linearidx_t t8_dpyramid_num_descendants_of_child_at_leveldiff(const t8_dpyramid_t *p, int childindex, int leveldiff)
{
  t8_dpyramid_t child;
  t8_dpyramid_child(p,childindex,&child);
  return t8_dpyramid_num_descendants_at_leveldiff(&child, leveldiff - 1);//????
}

static void
t8_dpyramid_init_linear_id_recursive (t8_dpyramid_t *p, const int level_diff,
                            t8_linearidx_t id)
{
  if(id == 0){
    t8_dpyramid_first_descendant(p,p,p->level+level_diff);
    return;
  }

  t8_linearidx_t sum_descendants_of_children_before = 0;
  t8_linearidx_t num_descendants_of_child = 0;
  int childindex;
  /*If needed, can be replaced by binary search in lookuptable*/
  for( childindex=0; childindex<t8_dpyramid_num_children(p); childindex++){
    num_descendants_of_child = t8_dpyramid_num_descendants_of_child_at_leveldiff(p,childindex, level_diff);
    if(sum_descendants_of_children_before + num_descendants_of_child > id) break;
    sum_descendants_of_children_before += num_descendants_of_child;
  }
  t8_dpyramid_child(p,childindex,p);
  t8_dpyramid_init_linear_id_recursive(p,level_diff-1, id - sum_descendants_of_children_before);
}

void
t8_dpyramid_init_linear_id (t8_dpyramid_t *p, const int level,
                            t8_linearidx_t id)
{
  t8_dpyramid_root(p);
  t8_dpyramid_init_linear_id_recursive(p, level, id);
}


t8_linearidx_t
t8_dpyramid_linear_id_recursive(t8_dpyramid_t *p, const t8_linearidx_t id, const int level_diff){
  if (p->level == 0) return id;

  const int childid = t8_dpyramid_child_id(p);
  t8_dpyramid_parent(p,p);
  t8_linearidx_t parent_id = 0;
  for (int ichild =0; ichild < childid; ichild++){
    /* p is now parent, so compute child to get sibling of original p */
    parent_id += t8_dpyramid_num_descendants_of_child_at_leveldiff(p,ichild,level_diff);
  }
  parent_id+=id;
  return t8_dpyramid_linear_id_recursive(p, parent_id, level_diff+1);
}

t8_linearidx_t
t8_dpyramid_linear_id (const t8_dpyramid_t *p, const int level)
{
  t8_dpyramid_t first_desc;
  t8_dpyramid_first_descendant(p,&first_desc, level);
  /* Maybe we can also input p into recursice function and calculate id directly for first desc */
  return t8_dpyramid_linear_id_recursive(&first_desc,0,0);
}




/* Vertex information */
void
t8_dpyramid_compute_coords (const t8_dpyramid_t *p, const int vertex,
                            int coords[])
{
  T8_ASSERT (0 <= vertex && vertex < t8_dpyramid_num_corners(p));
  for (int idim=0; idim < T8_DPYRAMID_DIM; idim++){
    t8_dpyramid_type_t type = t8_dpyramid_get_type(p);
    coords[idim] = t8_dpyramid_type_vertex_dim_to_binary[type][vertex][idim];
  }
}
/***VISUALISATION****/

void
t8_dpyramid_vertex_reference_coords (const t8_dpyramid_t *elem,
                                     const int vertex, double coords[])
{
  int                 coords_int[T8_DPYRAMID_DIM];
  T8_ASSERT (0 <= vertex && vertex < T8_DPYRAMID_MAX_CORNERS);
  t8_dpyramid_compute_coords (elem, vertex, coords_int);
  /*scale the coordinates onto the reference cube */
  coords[0] = coords_int[0] / (double) T8_DPYRAMID_ROOT_LEN;
  coords[1] = coords_int[1] / (double) T8_DPYRAMID_ROOT_LEN;
  coords[2] = coords_int[2] / (double) T8_DPYRAMID_ROOT_LEN;
}

/**
 * Compute the ancestor of \a pyra on level \a level.
 * 
 * \param[in]       pyra    Input pyramid
 * \param[in]       level   The level at which we want to compute \a anc
 * \param[in, out]  anc     Allocated input element which will be filled by the data of the anc of \a pyra at level \a level
 */
