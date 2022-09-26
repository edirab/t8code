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

/** This file provides helper functions for the cmesh vtk reader
*/
#include <t8_eclass.h>
#include <t8_cmesh.h>
#include <t8_cmesh_vtk_writer.h>
#include <t8_geometry/t8_geometry_implementations/t8_geometry_linear.h>
#include <t8_cmesh/t8_cmesh_reader_helper.hxx>
#include <t8_element_shape.h>

#if T8_WITH_VTK
#include <vtkCellIterator.h>
#include <vtkCellData.h>
#include <vtkUnstructuredGrid.h>
#include <vtkUnstructuredGridReader.h>
#include <vtkXMLUnstructuredGridReader.h>
#include <vtkXMLPUnstructuredGridReader.h>
#include <vtkPolyData.h>
#include <vtkBYUReader.h>
#include <vtkOBJReader.h>
#include <vtkPLYReader.h>
#include <vtkPolyDataReader.h>
#include <vtkSTLReader.h>
#include <vtkXMLPolyDataReader.h>

#include <t8_cmesh/t8_cmesh_vtk_helper.hxx>

const t8_eclass_t   t8_cmesh_vtk_type_to_t8_type[82] = {
  T8_ECLASS_INVALID, T8_ECLASS_VERTEX, T8_ECLASS_INVALID, T8_ECLASS_LINE,
  T8_ECLASS_INVALID, T8_ECLASS_TRIANGLE, T8_ECLASS_INVALID,
  T8_ECLASS_INVALID, T8_ECLASS_QUAD, T8_ECLASS_QUAD, T8_ECLASS_TET,
  T8_ECLASS_HEX, T8_ECLASS_HEX, T8_ECLASS_PRISM, T8_ECLASS_PYRAMID,
  T8_ECLASS_INVALID,
  T8_ECLASS_INVALID, T8_ECLASS_INVALID, T8_ECLASS_INVALID, T8_ECLASS_INVALID,
  T8_ECLASS_INVALID, T8_ECLASS_INVALID, T8_ECLASS_INVALID, T8_ECLASS_INVALID,
  T8_ECLASS_INVALID, T8_ECLASS_INVALID, T8_ECLASS_INVALID, T8_ECLASS_INVALID,
  T8_ECLASS_INVALID, T8_ECLASS_INVALID, T8_ECLASS_INVALID, T8_ECLASS_INVALID,
  T8_ECLASS_INVALID, T8_ECLASS_INVALID, T8_ECLASS_INVALID, T8_ECLASS_INVALID,
  T8_ECLASS_INVALID, T8_ECLASS_INVALID, T8_ECLASS_INVALID, T8_ECLASS_INVALID,
  T8_ECLASS_INVALID, T8_ECLASS_INVALID, T8_ECLASS_INVALID, T8_ECLASS_INVALID,
  T8_ECLASS_INVALID, T8_ECLASS_INVALID, T8_ECLASS_INVALID, T8_ECLASS_INVALID,
  T8_ECLASS_INVALID, T8_ECLASS_INVALID, T8_ECLASS_INVALID, T8_ECLASS_INVALID,
  T8_ECLASS_INVALID, T8_ECLASS_INVALID, T8_ECLASS_INVALID, T8_ECLASS_INVALID,
  T8_ECLASS_INVALID, T8_ECLASS_INVALID, T8_ECLASS_INVALID, T8_ECLASS_INVALID,
  T8_ECLASS_INVALID, T8_ECLASS_INVALID, T8_ECLASS_INVALID, T8_ECLASS_INVALID,
  T8_ECLASS_INVALID, T8_ECLASS_INVALID, T8_ECLASS_INVALID, T8_ECLASS_INVALID,
  T8_ECLASS_INVALID, T8_ECLASS_INVALID, T8_ECLASS_INVALID, T8_ECLASS_INVALID,
  T8_ECLASS_INVALID, T8_ECLASS_INVALID, T8_ECLASS_INVALID, T8_ECLASS_INVALID,
  T8_ECLASS_INVALID, T8_ECLASS_INVALID, T8_ECLASS_INVALID, T8_ECLASS_INVALID,
  T8_ECLASS_INVALID, T8_ECLASS_INVALID
};

vtkSmartPointer < vtkUnstructuredGrid >
t8_read_unstructured (const char *filename)
{
  char                tmp[BUFSIZ], *extension;
  /* Get the file-extension to decide which reader to use */
  strcpy (tmp, filename);
  extension = strtok (tmp, ".");
  extension = strtok (NULL, ".");

  /* Read the file */
  if (strcmp (extension, "vtu") == 0) {
    vtkSmartPointer < vtkXMLUnstructuredGridReader > reader =
      vtkSmartPointer < vtkXMLUnstructuredGridReader >::New ();
    if (!reader->CanReadFile (filename)) {
      t8_errorf ("Unable to read file.\n");
      return NULL;
    }
    reader->SetFileName (filename);
    reader->Update ();
    return reader->GetOutput ();
  }
  else if (strcmp (extension, "vtk") == 0) {
    vtkSmartPointer < vtkUnstructuredGridReader > reader =
      vtkSmartPointer < vtkUnstructuredGridReader >::New ();
    reader->SetFileName (filename);
    reader->Update ();
    if (!reader->IsFileUnstructuredGrid ()) {
      t8_errorf ("File-content is not an unstructured Grid. ");
    }
    return reader->GetOutput ();
  }
  else if (strcmp (extension, "pvtu") == 0) {
    vtkSmartPointer < vtkXMLPUnstructuredGridReader > reader =
      vtkSmartPointer < vtkXMLPUnstructuredGridReader >::New ();
    reader->SetFileName (filename);
    reader->Update ();
    t8_global_productionf
      ("You are using the serial reader to read a .pvtu file."
       " Every process will read the complete data. Use t8_cmesh_parallel_read_from_vtk_unstructured"
       " if the data should be read in chunks.\n");
    return reader->GetOutput ();
  }
  else {
    t8_global_errorf ("Please use .vtk, .vtu or .pvtu file\n");
    return NULL;
  }
}

vtkSmartPointer < vtkPolyData > t8_read_poly (const char *filename)
{
  char                tmp[BUFSIZ], *extension;
  /* Get the file-extension to decide which reader to use. */
  strcpy (tmp, filename);
  extension = strtok (tmp, ".");
  extension = strtok (NULL, ".");
  T8_ASSERT (strcmp (extension, ""));
  t8_debugf ("[D] %s\n", extension);
  /* Read the file depending on the extension */
  if (strcmp (extension, "ply") == 0) {
    vtkNew < vtkPLYReader > reader;
    reader->SetFileName (filename);
    reader->Update ();
    return reader->GetOutput ();
  }
  else if (strcmp (extension, "vtp") == 0) {
    vtkNew < vtkXMLPolyDataReader > reader;
    reader->SetFileName (filename);
    if (!reader->CanReadFile (filename)) {
      t8_errorf ("Unable to read file.\n");
      return NULL;
    }
    reader->Update ();
    return reader->GetOutput ();
  }
  else if (strcmp (extension, "obj") == 0) {
    vtkNew < vtkOBJReader > reader;
    reader->SetFileName (filename);
    reader->Update ();
    return reader->GetOutput ();
  }
  else if (strcmp (extension, "stl") == 0) {
    vtkNew < vtkSTLReader > reader;
    reader->SetFileName (filename);
    reader->Update ();
    return reader->GetOutput ();
  }
  else if (strcmp (extension, "vtk") == 0) {
    vtkNew < vtkPolyDataReader > reader;
    reader->SetFileName (filename);
    reader->Update ();
    if (!reader->IsFilePolyData ()) {
      t8_errorf ("File-content is not polydata.");
      return NULL;
    }
    return reader->GetOutput ();
  }
  else if (strcmp (extension, "g") == 0) {
    vtkNew < vtkBYUReader > reader;
    reader->SetGeometryFileName (filename);
    reader->Update ();
    return reader->GetOutput ();
  }
  else {
    /* Return NULL if the reader is not used correctly. */
    t8_global_errorf ("Please use .ply, .vtp, .obj, .stl, .vtk or .g file\n");
    return NULL;
  }
}

t8_gloidx_t
t8_vtk_iterate_cells (vtkSmartPointer < vtkDataSet > cells,
                      vtkSmartPointer < vtkCellData > cell_data,
                      sc_MPI_Comm comm, t8_cmesh_t cmesh)
{
  vtkCellIterator    *cell_it;
  vtkSmartPointer < vtkPoints > points;
  double             *vertices;
  double            **tuples;
  size_t             *data_size;
  t8_gloidx_t         tree_id = 0;
  t8_gloidx_t         first_tree_id;
  int                 max_dim = -1;

  first_tree_id = t8_cmesh_get_first_treeid (cmesh);
  if (first_tree_id < 0) {
    /*TODO: you can do that more elegant */
    first_tree_id = 0;
  }
  t8_debugf ("[D] iterate_cell fti: %li\n", first_tree_id);
  const int           max_cell_points = cells->GetMaxCellSize ();
  T8_ASSERT (max_cell_points > 0);
  vertices = T8_ALLOC (double, 3 * max_cell_points);
  /* Get cell iterator */
  cell_it = cells->NewCellIterator ();
  /* get the number of data-arrays per cell */
  const int           num_data_arrays = cell_data->GetNumberOfArrays ();
  T8_ASSERT (num_data_arrays >= 0);
  t8_debugf ("[D] read %i data-arrays\n", num_data_arrays);
  /* Prepare attributes */
  if (num_data_arrays > 0) {
    int                 tuple_size;
    tuples = T8_ALLOC (double *, num_data_arrays);
    data_size = T8_ALLOC (size_t, num_data_arrays);
    for (int idata = 0; idata < num_data_arrays; idata++) {
      vtkDataArray       *data = cell_data->GetArray (idata);
      tuple_size = data->GetNumberOfComponents ();
      data_size[idata] = sizeof (double) * tuple_size;
      /* Allocate memory for a tuple in array i */
      tuples[idata] = T8_ALLOC (double, tuple_size);
    }
  }

  /* Iterate over all cells */
  for (cell_it->InitTraversal (); !cell_it->IsDoneWithTraversal ();
       cell_it->GoToNextCell ()) {

    /* Set the t8_eclass of the cell */
    const t8_eclass_t   cell_type =
      t8_cmesh_vtk_type_to_t8_type[cell_it->GetCellType ()];
    SC_CHECK_ABORTF (t8_eclass_is_valid (cell_type),
                     "vtk-cell-type %i not supported by t8code\n", cell_type);
    t8_cmesh_set_tree_class (cmesh, tree_id + first_tree_id,
                             (t8_eclass_t) cell_type);
    /* Get the points of the cell */
    const int           num_points = cell_it->GetNumberOfPoints ();
    T8_ASSERT (num_points > 0);
    points = cell_it->GetPoints ();
    for (int ipoint = 0; ipoint < num_points; ipoint++) {
      points->GetPoint (t8_element_shape_vtk_corner_number
                        (cell_type, ipoint), &vertices[3 * ipoint]);
    }
    /* The order of the vertices in vtk might give a tree with negative volume */
    if (t8_cmesh_tree_vertices_negative_volume
        (cell_type, vertices, num_points)) {
      t8_cmesh_correct_volume (vertices, cell_type);
    }
    t8_cmesh_set_tree_vertices (cmesh, tree_id + first_tree_id, vertices,
                                num_points);

    /* Get and set the data of each cell */
    for (int dtype = 0; dtype < num_data_arrays; dtype++) {
      const t8_gloidx_t   cell_id = cell_it->GetCellId ();
      vtkDataArray       *data = cell_data->GetArray (dtype);
      data->GetTuple (cell_id, tuples[dtype]);
      t8_cmesh_set_attribute (cmesh, cell_id, t8_get_package_id (),
                              dtype + 1, tuples[dtype], data_size[dtype], 0);
    }
    /* Check geometry-dimension */
    if (max_dim < cell_it->GetCellDimension ()) {
      max_dim = cell_it->GetCellDimension ();
    }
    tree_id++;
  }
  t8_debugf ("[D] read %li trees\n", tree_id);
  /*Set the geometry */
  t8_geometry_c      *linear_geom = t8_geometry_linear_new (max_dim);
  t8_cmesh_register_geometry (cmesh, linear_geom);

  /* Clean-up */
  cell_it->Delete ();
  if (num_data_arrays > 0) {
    T8_FREE (data_size);
    for (int idata = num_data_arrays - 1; idata >= 0; idata--) {
      T8_FREE (tuples[idata]);
    }
    T8_FREE (tuples);
  }
  T8_FREE (vertices);
  return tree_id;
}

#endif
