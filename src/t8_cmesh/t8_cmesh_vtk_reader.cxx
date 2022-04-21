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

#include <t8_cmesh.h>
#include <t8_cmesh_vtk_writer.h>
#include <t8_cmesh/t8_cmesh_vtk_reader.hxx>

#if T8_WITH_VTK
#include <vtkUnstructuredGrid.h>
#include <vtkUnstructuredGridReader.h>
#include <vtkXMLUnstructuredGridReader.h>
#include <vtkCellIterator.h>
#endif
T8_EXTERN_C_BEGIN ();

/*Construct a cmesh given a filename and a*/
t8_cmesh_t
t8_cmesh_read_from_vtk (const char *filename, const int num_files,
                        sc_MPI_Comm comm)
{
  t8_cmesh_t          cmesh;
#if T8_WITH_VTK
  /*The Incoming data must be an unstructured Grid */
  t8_eclass_t         cell_type;
  vtkSmartPointer < vtkUnstructuredGrid > unstructuredGrid;
  vtkCellIterator    *cell_it;
  char               *tmp, *extension;
  int                 max_dim = 0;      /*max dimenstion of the cells for geometry */
  int                 num_cell_points, max_cell_points;
  vtkSmartPointer < vtkPoints > points =
    vtkSmartPointer < vtkPoints >::New ();
  double             *vertices;
  /*Get the file-extensioni to decide which reader to use */
  tmp = T8_ALLOC (char, BUFSIZ);
  strcpy (tmp, filename);
  extension = strtok (tmp, ".");
  extension = strtok (NULL, ".");
  T8_FREE (tmp);
  T8_ASSERT (strcmp (extension, ""));

  /*Read the file */
  if (strcmp (extension, "vtu") == 0) {
    t8_debugf ("[D] use xml unstructured\n");
    vtkSmartPointer < vtkXMLUnstructuredGridReader > reader =
      vtkSmartPointer < vtkXMLUnstructuredGridReader >::New ();
    reader->SetFileName (filename);
    reader->Update ();
    unstructuredGrid = reader->GetOutput ();
  }
  else if (strcmp (extension, "vtk") == 0) {
    t8_debugf ("[D] use unstructured\n");
    vtkSmartPointer < vtkUnstructuredGridReader > reader =
      vtkSmartPointer < vtkUnstructuredGridReader >::New ();
    reader->SetFileName (filename);
    reader->Update ();
    unstructuredGrid = reader->GetOutput ();
  }
  else {
    t8_global_errorf ("Please use .vtk or .vtu file\n");
  }

  t8_cmesh_init (&cmesh);

  t8_debugf ("[D] num_cells: %lli\n", unstructuredGrid->GetNumberOfCells ());
  /*New Iteratore to iterate over all cells in the grid */
  cell_it = unstructuredGrid->NewCellIterator ();
  max_cell_points = unstructuredGrid->GetMaxCellSize ();
  vertices = T8_ALLOC (double, 3 * max_cell_points);
  t8_gloidx_t         tree_id = 0;
  for (cell_it->InitTraversal (); !cell_it->IsDoneWithTraversal ();
       cell_it->GoToNextCell ()) {
    /*Set the t8_eclass of the cell */
    cell_type = t8_cmesh_vtk_type_to_t8_type[cell_it->GetCellType ()];
    T8_ASSERT (cell_type != T8_ECLASS_ZERO);
    t8_debugf ("[D] cell has type %i\n", cell_type);
    t8_cmesh_set_tree_class (cmesh, tree_id, cell_type);

    /*Set the vertices of the tree */
    num_cell_points = cell_it->GetNumberOfPoints ();
    t8_debugf ("[D] cell has %i points\n", num_cell_points);
    points = cell_it->GetPoints ();
    for (int i = 0; i < num_cell_points; i++) {
      points->GetPoint (i, &vertices[3 * i]);
      t8_debugf ("[D] %i: %f %f %f\n", i, vertices[3 * i],
                 vertices[3 * i + 1], vertices[3 * i + 2]);
    }
    t8_cmesh_set_tree_vertices (cmesh, tree_id, vertices, num_cell_points);

    tree_id++;
  }
  t8_cmesh_commit (cmesh, comm);
#else
  /*TODO: Proper return value to prevent compiler-errors */
  t8_global_errorf
    ("WARNING: t8code is not linked against the vtk library. Without proper linking t8code cannot use the vtk-reader\n");
#endif
  return cmesh;
}

T8_EXTERN_C_END ();
