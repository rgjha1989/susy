// -----------------------------------------------------------------
// Main procedure for N=4 SYM measurements on saved configurations,
// including scalar correlators, Wilson loops, Ward identity violations
// and discrete R symmetry observables
#define CONTROL
#include "susy_includes.h"
// -----------------------------------------------------------------



// -----------------------------------------------------------------
int main(int argc, char *argv[]) {
  int prompt, dir;
  double dssplaq, dstplaq, dtime, plpMod = 0.0;
  double linktr[NUMLINK], linktr_ave, linktr_width;
  double link_det[NUMLINK], det_ave, det_width;
  complex plp = cmplx(99.0, 99.0);

  // Setup
  setlinebuf(stdout); // DEBUG
  initialize_machine(&argc, &argv);
  // Remap standard I/O
  if (remap_stdio_from_args(argc, argv) == 1)
    terminate(1);

  g_sync();
  prompt = setup();
  setup_lambda();
  epsilon();
  setup_PtoP();
  setup_FQ();

#ifdef WLOOP
  register int i;
  register site *s;
#endif

#ifdef PL_CORR
  // Set up Fourier transform for Polyakov loop correlator
  int key[4] = {1, 1, 1, 0};
  int restrict[4];
  Real space_vol = (Real)(nx * ny * nz);
  setup_restrict_fourier(key, restrict);
#endif

  // Load input and run
  if (readin(prompt) != 0) {
    node0_printf("ERROR in readin, aborting\n");
    terminate(1);
  }
  dtime = -dclock();

  // Uncomment this block to print plaquette, determinant & trace distributions
  // Be sure to uncomment PLAQ_DIST and DET_DIST, and run in serial
//  d_plaquette_lcl(&dssplaq, &dstplaq);
//  node0_printf("\n");
//  if (G < IMAG_TOL)
//    G = 999;
//  if (B < IMAG_TOL)
//    B = 999;
//  compute_DmuUmu();
//  dtime += dclock();
//  node0_printf("\nTime = %.4g seconds\n", dtime);
//  fflush(stdout);
//  return 0;

  // Check: compute initial plaquette and bosonic action
  d_plaquette(&dssplaq, &dstplaq);
  node0_printf("START %.8g %.8g %.8g ", dssplaq, dstplaq, dssplaq + dstplaq);
  dssplaq = d_gauge_action(NODET);
  node0_printf("%.8g\n", dssplaq / (double)volume);

  // Do "local" measurements to check configuration
  // Tr[Udag.U] / N
  linktr_ave = d_link(linktr, &linktr_width, link_det, &det_ave, &det_width);
  node0_printf("FLINK");
  for (dir = XUP; dir < NUMLINK; dir++)
    node0_printf(" %.6g", linktr[dir]);
  node0_printf(" %.6g %.6g\n", linktr_ave, linktr_width);
  node0_printf("FLINK_DET");
  for (dir = XUP; dir < NUMLINK; dir++)
    node0_printf(" %.6g", link_det[dir]);
  node0_printf(" %.6g %.6g\n", det_ave, det_width);

  // Polyakov loop and plaquette measurements
  // Format: GMES Re(Polyakov) Im(Poyakov) cg_iters ss_plaq st_plaq
  plp = ploop(&plpMod);
  d_plaquette(&dssplaq, &dstplaq);
  node0_printf("GMES %.8g %.8g 0 %.8g %.8g ",
               plp.real, plp.imag, dssplaq, dstplaq);

  // Bosonic action (printed twice by request)
  // Might as well spit out volume average of Polyakov loop modulus
  dssplaq = d_gauge_action(NODET) / (double)volume;
//  dssplaq = d_gauge_action(YESDET) / (double)volume;
  node0_printf("%.8g ", dssplaq);
  node0_printf("%.8g\n", plpMod);
  node0_printf("BACTION %.8g\n", dssplaq);

#ifdef SMEAR
  // Optionally smear before main measurements
  node0_printf("Doing %d polar-projected APE smearing steps ", Nsmear);
  node0_printf("with alpha=%.4g\n", alpha);

  // Check minimum plaquette in addition to averages
  node0_printf("BEFORE ");
  d_plaquette_lcl(&dssplaq, &dstplaq);    // Prints out MIN_PLAQ
  node0_printf(" %.8g %.8g\n", dssplaq, dstplaq);

  // Overwrite s->linkf
  APE_smear(Nsmear, alpha, YESDET);
  node0_printf("AFTER  ");
  d_plaquette_lcl(&dssplaq, &dstplaq);    // Prints out MIN_PLAQ
  node0_printf(" %.8g %.8g\n", dssplaq, dstplaq);
#endif

  // Main measurements
  // Plaquette determinant
  measure_det();

  // Monitor widths of plaquette and plaquette determinant distributions
  widths();

#ifdef PL_CORR
  // Polyakov loop correlator
  // Compute ploop_corr and take the absolute square of its FFT
  ploop_c();      // Computes Polyakov loop at each spatial site
  restrict_fourier(F_OFFSET(ploop_corr),
                   F_OFFSET(fft1), F_OFFSET(fft2),
                   sizeof(complex), FORWARDS);

  FORALLSITES(i, s)
    CMULJ_((s->ploop_corr), (s->ploop_corr), (s->print_var));

  // Invert FFT in place
  restrict_fourier(F_OFFSET(print_var),
                   F_OFFSET(fft1), F_OFFSET(fft2),
                   sizeof(complex), BACKWARDS);

  // Divide by volume for correct inverse FFT
  // Divide by another volume to average convolution
  FORALLSITES(i, s)
    CDIVREAL((s->print_var), space_vol * space_vol, (s->print_var));

  print_var3("PLCORR");
#endif

#ifdef CORR
  // Konishi and SUGRA correlators
  d_konishi();
  d_correlator_r();
#endif

#ifdef BILIN
  // Ward identity violations
  int avm_iters = d_susyTrans();

  // Don't run d_bilinear() for now
  // In the future it may be useful
  // to compare U(1) vs. SU(N) contributions
//  avm_iters += d_bilinear();
#endif

#ifdef CORR
  // R symmetry transformations -- use find_det and adjugate
  rsymm(YESDET);

  // Measure density of monopole world lines in non-diagonal cubes
  monopole();
#endif

#ifdef WLOOP
  // Gauge fix to Coulomb gauge
  // This lets us easily access arbitrary displacements
  if (fixflag == COULOMB_GAUGE_FIX) {
    d_plaquette(&dssplaq, &dstplaq);    // To be printed below
    node0_printf("Fixing to Coulomb gauge...\n");
    double gtime = -dclock();
    gaugefix(TUP, 1.5, 5000, GAUGE_FIX_TOL, -1, -1);
    gtime += dclock();
    node0_printf("GFIX time = %.4g seconds\n", gtime);
    node0_printf("BEFORE %.8g %.8g\n", dssplaq, dstplaq);
    d_plaquette(&dssplaq, &dstplaq);
    node0_printf("AFTER  %.8g %.8g\n", dssplaq, dstplaq);
  }
  hvy_pot(NODET);
  hvy_pot(YESDET);

  // Save and restore links overwritten by polar projection
  FORALLSITES(i, s)
    su3mat_copy_f(&(s->linkf[TUP]), &(s->mom[TUP]));
  hvy_pot_polar();
  FORALLSITES(i, s)
    su3mat_copy_f(&(s->mom[TUP]), &(s->linkf[TUP]));
#endif

  node0_printf("RUNNING COMPLETED\n");
#ifdef BILIN
  node0_printf("CG iters for measurements: %d\n", avm_iters);
  // total_iters is accumulated in the multiCG itself
  // Should equal total for bilinear measurements
  node0_printf("total_iters = %d\n", total_iters);
#endif
  dtime += dclock();
  node0_printf("\nTime = %.4g seconds\n", dtime);
  fflush(stdout);
  g_sync();         // Needed by at least some clusters
  return 0;
}
// -----------------------------------------------------------------
