#include <t8.h>
#include <t8_cmesh.h>
#include <t8_cmesh/t8_cmesh_examples.h>
#include <t8_forest.h>
#include <t8_schemes/t8_default/t8_default_cxx.hxx>
#include <t8_vec.h>
#include "t8_cmesh/t8_cmesh_testcases.h"

T8_EXTERN_C_BEGIN ();

struct t8_adapt_data
{
  double  midpoint[6][3];
};

int
t8_adapt_callback_init (t8_forest_t forest,
                        t8_forest_t forest_from,
                        t8_locidx_t which_tree,
                        t8_locidx_t lelement_id,
                        t8_eclass_scheme_c * ts,
                        const int is_family,
                        const int num_elements,  
                        t8_element_t * elements[])
{
  const struct t8_adapt_data *adapt_data = (const struct t8_adapt_data *) t8_forest_get_user_data (forest);
  
  double  centroid[3];
  double  dist;

  T8_ASSERT (adapt_data != NULL);

  t8_forest_element_centroid (forest_from, which_tree, elements[0], centroid);
  
  /* remove every element outside an central ball of radius 0.65 */
  //double center[3] = {0.5, 0.5, 0.5};
  //dist = t8_vec_dist(center, centroid);
  //if (dist > 0.5) {
  //  return -2;
  //}

  for (int i = 0; i < 6; i++) {
    dist = t8_vec_dist(adapt_data->midpoint[i], centroid);
    /* remove core of every ball */
    if (dist < 0.4) {
      return -2;
    }
    /* refine shell of every ball */
    if (dist < 0.5) {
      return 1;
    }
  }

  return 0;
}

int
t8_adapt_callback_remove (t8_forest_t forest,
                          t8_forest_t forest_from,
                          t8_locidx_t which_tree,
                          t8_locidx_t lelement_id,
                          t8_eclass_scheme_c * ts,
                          const int is_family,
                          const int num_elements, 
                          t8_element_t * elements[])
{
  const struct t8_adapt_data *adapt_data = (const struct t8_adapt_data *) t8_forest_get_user_data (forest);
  
  double  centroid[3];
  double  dist;
  
  T8_ASSERT (adapt_data != NULL);

  t8_forest_element_centroid (forest_from, which_tree, elements[0], centroid);
  
  for (int i = 0; i < 6; i++) {
    dist = t8_vec_dist(adapt_data->midpoint[i], centroid);
    if (dist < 0.45) {
      return -2;
    }
  }

  return 0;
}

/* Coarse if at least one element of a family is within a given radius. */
int
t8_adapt_callback_coarse (t8_forest_t forest,
                          t8_forest_t forest_from,
                          t8_locidx_t which_tree,
                          t8_locidx_t lelement_id,
                          t8_eclass_scheme_c * ts,
                          const int is_family,
                          const int num_elements, 
                          t8_element_t * elements[])
{
  const struct t8_adapt_data *adapt_data = (const struct t8_adapt_data *) t8_forest_get_user_data (forest);
  
  double  centroid[3];
  double  dist;

  T8_ASSERT (adapt_data != NULL);
  
  if (is_family) {
    /* all Balls */
    for (int i = 0; i < 6; i++) {
      /* all member of family */
      for (int j = 0; j < num_elements; j++) {
        t8_forest_element_centroid (forest_from, which_tree, elements[j], centroid);
        dist = t8_vec_dist(adapt_data->midpoint[i], centroid);
        /* if one family member satisfies the condition, coarse */
        if (dist < 0.5) {
          return -1;
        }
      }
    }
  }
  return 0;
}

int
t8_adapt_callback_refine (t8_forest_t forest,
                          t8_forest_t forest_from,
                          t8_locidx_t which_tree,
                          t8_locidx_t lelement_id,
                          t8_eclass_scheme_c * ts,
                          const int is_family,
                          const int num_elements, 
                          t8_element_t * elements[])
{
  const struct t8_adapt_data *adapt_data = (const struct t8_adapt_data *) t8_forest_get_user_data (forest);
  
  double  centroid[3];
  double  dist;

  T8_ASSERT (adapt_data != NULL);

  t8_forest_element_centroid (forest_from, which_tree, elements[0], centroid);
  

  for (int i = 0; i < 6; i++) {
    dist = t8_vec_dist(adapt_data->midpoint[i], centroid);
    if (dist < 0.5) {
      return 1;
    }
  }

  return 0;
}

int
t8_adapt_callback_coarse_all (t8_forest_t forest,
                              t8_forest_t forest_from,
                              t8_locidx_t which_tree,
                              t8_locidx_t lelement_id,
                              t8_eclass_scheme_c * ts,
                              const int is_family,
                              const int num_elements, 
                              t8_element_t * elements[])
{
  if(is_family) {
    return -1;
  }
  return 0;
}

int
main (int argc, char **argv)
{
  int                 mpiret;
  sc_MPI_Comm         comm;
  t8_cmesh_t          cmesh;
  t8_forest_t         forest, forest_1, forest_2;

  const int           level = 4;

  mpiret = sc_MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);

  sc_init (sc_MPI_COMM_WORLD, 1, 1, NULL, SC_LP_ESSENTIAL);
  t8_init (SC_LP_DEFAULT);

  comm = sc_MPI_COMM_WORLD;

  //cmesh = t8_cmesh_new_hypercube (T8_ECLASS_HEX, comm, 0, 0, 0);
  cmesh = t8_cmesh_new_hypercube_hybrid (comm, 0, 0);
  //cmesh = t8_test_create_cmesh (100); 
  forest = t8_forest_new_uniform (cmesh, t8_scheme_new_default_cxx (), level, 0, comm);
  T8_ASSERT (t8_forest_is_committed (forest));

  struct t8_adapt_data adapt_data = {{{1.0, 0.5, 0.5},
                                      {0.5, 1.0, 0.5},
                                      {0.5, 0.5, 1.0},
                                      {0.0, 0.5, 0.5},
                                      {0.5, 0.0, 0.5},
                                      {0.5, 0.5, 0.0}}};
  
  forest_1 = t8_forest_new_adapt (forest  , t8_adapt_callback_init  , 0, 0, &adapt_data);
  forest_1 = t8_forest_new_adapt (forest_1, t8_adapt_callback_remove, 0, 0, &adapt_data);
  
  t8_forest_write_vtk (forest_1, "/home/ioannis/VBshare/paraview_export/t8_example_1");

  t8_forest_ref (forest_1);
  forest_2 = t8_forest_new_adapt (forest_1, t8_adapt_callback_coarse, 0, 0, &adapt_data);
  forest_2 = t8_forest_new_adapt (forest_2, t8_adapt_callback_refine, 0, 0, &adapt_data);
  forest_2 = t8_forest_new_adapt (forest_2, t8_adapt_callback_remove, 0, 0, &adapt_data);

  t8_global_productionf("Test\n");
  if (t8_forest_is_equal (forest_1, forest_2)) {
    t8_global_productionf("Forests are equal.\n");
  }
  else {
    t8_global_productionf("Forests are not equal.\n");
  }
  //for (int i = 0; i < level+1; i++)
  //{
  //  forest_2 = t8_forest_new_adapt (forest_2, t8_adapt_callback_coarse_all, 0, 0, &adapt_data);
  //}
  //t8_forest_write_vtk (forest_2, "t8_example_2");

  //t8_forest_unref (&forest);
  t8_forest_unref (&forest_1);
  t8_forest_unref (&forest_2);
  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}

T8_EXTERN_C_END ();
