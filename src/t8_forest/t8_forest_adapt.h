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

/** \file t8_forest_adapt.h
 * We define the adapt routine to refine and corsen a forest of trees in this file.
 */

/* TODO: begin documenting this file: make doxygen 2>&1 | grep t8_forest_adapt */

#ifndef T8_FOREST_ADAPT_H
#define T8_FOREST_ADAPT_H

#include <t8.h>
#include <t8_forest.h>

T8_EXTERN_C_BEGIN ();

/* TODO: comment */
void                t8_forest_adapt (t8_forest_t forest);

/** A refinement callback function that works with a marker array.
 * We expect the forest user_data pointer to point to an sc_array of short ints.
 * We expect one int per element, which is interpreted as follows:
 *    0 - this element should neither be refined nor coarsened
 *    1 - refine this element
 *   -1 - coarsen this elements (must be set for the whole family)
 * other values are invalid.
 * \param [in] forest      the forest to which the new elements belong
 * \param [in] forest_from the forest that is adapted.
 * \param [in] which_tree  the local tree containing \a elements
 * \param [in] lelement_id the local element id in \a forest_old in the tree of the current element
 * \param [in] ts          the eclass scheme of the tree
 * \param [in] num_elements the number of entries in \a elements
 * \param [in] elements    Pointers to a family or, if second entry is NULL,
 *                         pointer to one element.
 * \return greater zero if the first entry in \a elements should be refined,
 *         smaller zero if the family \a elements shall be coarsened,
 *         zero else.
 */
int                 t8_forest_adapt_marker_array_callback (t8_forest_t forest,
                                                           t8_forest_t
                                                           forest_from,
                                                           t8_locidx_t
                                                           which_tree,
                                                           t8_locidx_t
                                                           lelement_id,
                                                           t8_eclass_scheme_c
                                                           * ts,
                                                           int num_elements,
                                                           t8_element_t *
                                                           elements[]);

T8_EXTERN_C_END ();

#endif /* !T8_FOREST_ADAPT_H! */
