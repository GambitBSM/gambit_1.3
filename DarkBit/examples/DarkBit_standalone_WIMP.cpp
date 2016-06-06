//   GAMBIT: Global and Modular BSM Inference Tool
//   *********************************************
///  \file
///
///  Example of GAMBIT DarkBit standalone
///  main program.
///
///  *********************************************
///
///  Authors (add name and date if you modify):
///   
///  \author Christoph Weniger
///  \date 2016 Feb
///
///  *********************************************

// Always required in any standalone module main file
#include "gambit/Utils/standalone_module.hpp"
#include "gambit/DarkBit/DarkBit_rollcall.hpp"
#include "gambit/Elements/spectrum_factories.hpp"
#include <boost/multi_array.hpp>
#include <iostream>
#include <fstream>

// Only needed here
#include "gambit/Utils/util_functions.hpp"

using namespace DarkBit::Functown;     // Functors wrapping the module's actual module functions
using namespace DarkBit::Accessors;    // Helper functions that provide some info about the module
using namespace BackendIniBit::Functown;    // Functors wrapping the backend initialisation functions

// FIXME: Make sure to have meaningful error messages when backends are not correctly loaded 

QUICK_FUNCTION(DarkBit, TH_ProcessCatalog, OLD_CAPABILITY, TH_ProcessCatalog_WIMP, DarkBit::TH_ProcessCatalog, ())
QUICK_FUNCTION(DarkBit, DarkMatter_ID, OLD_CAPABILITY, DarkMatter_ID_WIMP, std::string, ())
QUICK_FUNCTION(DarkBit, DD_couplings, OLD_CAPABILITY, DD_couplings_WIMP, DM_nucleon_couplings, ())

void dump_array_to_file(const std::string & filename, const
    boost::multi_array<double, 2> & a, const std::vector<double> & x, const
    std::vector<double> & y)
{
  std::fstream file;
  file.open(filename, std::ios_base::out);
  file << "0.0 ";
  for (size_t i = 0; i < x.size(); i++)
    file << x[i] << " ";
  file << std::endl;
  for (size_t j = 0; j < y.size(); j++)
  {
    file << y[j] << " ";
    for (size_t i = 0; i < x.size(); i++)
    {
      file << a[i][j] << " ";
    }
    file << std::endl;
  }
  file.close();
}

void dumpSpectrum(std::string filename, double mWIMP, double sv, std::vector<double> brList)
{
  DarkMatter_ID_WIMP.reset_and_calculate();
  TH_ProcessCatalog_WIMP.setOption<std::vector<double>>("brList", brList);
  TH_ProcessCatalog_WIMP.setOption<double>("mWIMP", mWIMP);
  TH_ProcessCatalog_WIMP.setOption<double>("sv", sv);
  TH_ProcessCatalog_WIMP.reset_and_calculate();
  RD_fraction_fixed.reset_and_calculate();
  SimYieldTable_DarkSUSY.reset_and_calculate();
  GA_missingFinalStates.reset_and_calculate();
  cascadeMC_FinalStates.reset_and_calculate();
  cascadeMC_DecayTable.reset_and_calculate();
  cascadeMC_LoopManager.reset_and_calculate();
  cascadeMC_gammaSpectra.reset_and_calculate();
  GA_AnnYield_General.reset_and_calculate();
  dump_GammaSpectrum.setOption<std::string>("filename", filename);
  dump_GammaSpectrum.reset_and_calculate();
}

namespace Gambit
{
  namespace DarkBit
  {
    
    void TH_ProcessCatalog_WIMP(DarkBit::TH_ProcessCatalog& result)
    {
      using namespace Pipes::TH_ProcessCatalog_WIMP;
      using std::vector;
      using std::string;

      // Initialize empty catalog and main annihilation process
      TH_ProcessCatalog catalog;
      TH_Process process_ann((string)"WIMP", (string)"WIMP");
      TH_Process process_dec((string)"phi");
      TH_Process process_dec1((string)"phi1");
      TH_Process process_dec2((string)"phi2");

      ///////////////////////////////////////
      // Import particle masses and couplings
      ///////////////////////////////////////
      
#define addParticle(Name, Mass, spinX2)                                        \
      catalog.particleProperties.insert(std::pair<string, TH_ParticleProperty> \
      (Name , TH_ParticleProperty(Mass, spinX2)));    

      double mWIMP = runOptions->getValue<double>("mWIMP");
      double sv = runOptions->getValue<double>("sv");
      double b = 0;  // defined as sv(v) = sv(v=0) + b*(sv=0)*v**2
      auto brList = runOptions->getValue<std::vector<double>>("brList");

      // FIXME: Use various channels include 3-body and complicated cascade
      // decay
      // FIXME: Check stability of codes w.r.t. extreme parameters
      // FIXME: Test all input possible for this function
      addParticle("gamma", 0.0,  2)
      addParticle("Z0", 91.2,  2)
      addParticle("tau+", 1.8,  1)
      addParticle("tau-", 1.8,  1)
      addParticle("b", 4.9,  1)
      addParticle("bbar", 4.9,  1)

      addParticle("WIMP", mWIMP,  0)
      addParticle("phi",  59.0,  0)
      addParticle("phi1", 99.99,  0)
      addParticle("phi2", 49.99,  0)
#undef addParticle

      TH_Channel dec_channel(Funk::vec<string>("gamma", "gamma"), Funk::cnst(1.));
      process_dec.channelList.push_back(dec_channel);

      TH_Channel dec_channel1(Funk::vec<string>("phi2", "phi2"), Funk::cnst(1.));
      process_dec1.channelList.push_back(dec_channel1);

      TH_Channel dec_channel2(Funk::vec<string>("gamma", "gamma"), Funk::cnst(1.));
      process_dec2.channelList.push_back(dec_channel2);

      process_ann.thresholdResonances.threshold_energy.push_back(2*mWIMP); 
      auto p1 = Funk::vec<string>("b", "gamma", "gamma", "phi", "phi1");
      auto p2 = Funk::vec<string>("bbar", "Z0", "gamma", "phi", "phi1");
      {
        for ( unsigned int i = 0; i < brList.size()-1; i++ )
        {
          double mtot_final = 
            catalog.getParticleProperty(p1[i]).mass +
            catalog.getParticleProperty(p2[i]).mass;
          if ( mWIMP*2 > mtot_final * 0)
          {
            Funk::Funk kinematicFunction = (Funk::one("v")+pow(Funk::var("v"), 2)*b)*sv*brList[i];
            TH_Channel new_channel(
                Funk::vec<string>(p1[i], p2[i]), kinematicFunction
                );
            process_ann.channelList.push_back(new_channel);
          }
          else
          {
            process_ann.thresholdResonances.threshold_energy.
              push_back(mtot_final);
          }
        }
      }

      if ( brList[5] > 0. )
      {
        auto E = Funk::var("E");
        Funk::Funk kinematicFunction = Funk::one("v", "E1")/(pow(E-50, 2)+1)*sv*brList[5];
        // FIXME: Include second gamma in AnnYield (currently ignored)
        TH_Channel new_channel(Funk::vec<string>("gamma", "gamma", "Z0"), kinematicFunction);
        process_ann.channelList.push_back(new_channel);
      }

      catalog.processList.push_back(process_ann);
      catalog.processList.push_back(process_dec);
      catalog.processList.push_back(process_dec1);
      catalog.processList.push_back(process_dec2);

      catalog.validate();

      result = catalog;
    } // function TH_ProcessCatalog_WIMP

    void DarkMatter_ID_WIMP(std::string& result)
    {
      result = "WIMP";
    }

    void DD_couplings_WIMP(DM_nucleon_couplings& result)
    {
      using namespace Pipes::DD_couplings_WIMP;
      result.gps = runOptions->getValueOrDef<double>(0., "gps");
      result.gns = runOptions->getValueOrDef<double>(0., "gns");
      result.gpa = runOptions->getValueOrDef<double>(0., "gpa");
      result.gna = runOptions->getValueOrDef<double>(0., "gna");
      std::cout << "DD_coupling says" << std::endl;
      std::cout << result.gps << std::endl;
    }
  }
}

int main(int argc, char* argv[])
{
  if (argc==1)
  {
    std::cout << "Please select test mode (1-7)" << std::endl;
    exit(1);
  }
  int mode = std::stoi((std::string)argv[1]);
  std::cout << "Starting with mode " << mode << std::endl;


  // ---- Initialise (or disable) logging ----

  std::map<std::string, std::string> loggerinfo;
  std::string prefix("runs/DarkBit_standalone_WIMP/logs/");
  Utils::ensure_path_exists(prefix);

  loggerinfo["Core, Error"] = prefix+"core_errors.log";
  loggerinfo["Default"]     = prefix+"default.log";
  loggerinfo["Debug"]       = prefix+"debug.log";
  loggerinfo["Warning"]     = prefix+"warnings.log";
  loggerinfo["DarkBit, Info"] = prefix+"DarkBit_info.log";

  logger().initialise(loggerinfo);

  model_warning().set_fatal(true);
  DarkBit::DarkBit_error().set_fatal(true);

  logger()<<"Running DarkBit standalone example"<<LogTags::info<<EOM;


  // ---- Initialize models ----

  // Initialize LocalHalo model
  ModelParameters* LocalHalo_primary_parameters = Models::LocalHalo::Functown::primary_parameters.getcontentsPtr();
  LocalHalo_primary_parameters->setValue("rho0", 0.4);
  LocalHalo_primary_parameters->setValue("vrot", 235.);
  LocalHalo_primary_parameters->setValue("v0", 235.);
  LocalHalo_primary_parameters->setValue("vesc", 550.);
  LocalHalo_primary_parameters->setValue("vearth", 29.78);

  // Set up DDCalc backend initialization
  Backends::DDCalc_1_0_0::Functown::DDCalc_CalcRates_simple.setStatus(2);
  Backends::DDCalc_1_0_0::Functown::DDCalc_Experiment.setStatus(2);
  Backends::DDCalc_1_0_0::Functown::DDCalc_LogLikelihood.setStatus(2);
  DDCalc_1_0_0_init.notifyOfModel("LocalHalo");
  DDCalc_1_0_0_init.resolveDependency(&Models::LocalHalo::Functown::primary_parameters);
  DDCalc_1_0_0_init.resolveDependency(&RD_fraction_fixed);
  DDCalc_1_0_0_init.resolveDependency(&mwimp_generic);
  DDCalc_1_0_0_init.resolveDependency(&DD_couplings_WIMP); // Use DarkSUSY for DD couplings


  // ---- Set up basic internal structures for direct & indirect detection ----

  // Set identifier for DM particle
  // FIXME: Needed?

  // Set up process catalog based on DarkSUSY annihilation rates

  // Assume for direct and indirect detection likelihoods that dark matter
  // density is always the measured one (despite relic density results)


  // ---- Gamma-ray yields ----

  // Initialize tabulated gamma-ray yields
  // FIXME: Use three different simyieldtables
  SimYieldTable_DarkSUSY.resolveBackendReq(&Backends::DarkSUSY_5_1_3::Functown::dshayield);

  // Collect missing final states for simulation in cascade MC
  GA_missingFinalStates.resolveDependency(&TH_ProcessCatalog_WIMP);
  GA_missingFinalStates.resolveDependency(&SimYieldTable_DarkSUSY);
  GA_missingFinalStates.resolveDependency(&DarkMatter_ID_WIMP);

  // Infer for which type of final states particles MC should be performed
  cascadeMC_FinalStates.setOption<std::vector<std::string>>("cMC_finalStates", Funk::vec((std::string)"gamma"));

  // Collect decay information for cascade MC
  cascadeMC_DecayTable.resolveDependency(&TH_ProcessCatalog_WIMP);
  cascadeMC_DecayTable.resolveDependency(&SimYieldTable_DarkSUSY);

  // Set up MC loop manager for cascade MC
  // FIXME: Systematically test accuracy and dependence on setup parameters
  // FIXME: Add maximum width for energy bins
  cascadeMC_LoopManager.setOption<int>("cMC_maxEvents", 10);
  cascadeMC_LoopManager.resolveDependency(&GA_missingFinalStates);
  cascadeMC_LoopManager.resolveDependency(&cascadeMC_DecayTable);
  cascadeMC_LoopManager.resolveDependency(&SimYieldTable_DarkSUSY);
  cascadeMC_LoopManager.resolveDependency(&TH_ProcessCatalog_WIMP);
  std::vector<functor*> nested_functions = initVector<functor*>(
      &cascadeMC_InitialState, &cascadeMC_GenerateChain, &cascadeMC_Histograms, &cascadeMC_EventCount);
  cascadeMC_LoopManager.setNestedList(nested_functions);

  // Set up initial state for cascade MC step
  cascadeMC_InitialState.resolveDependency(&GA_missingFinalStates);
  cascadeMC_InitialState.resolveLoopManager(&cascadeMC_LoopManager);
  //cascadeMC_InitialState.reset_and_calculate();

  // Perform MC step for cascade MC
  cascadeMC_GenerateChain.resolveDependency(&cascadeMC_InitialState);
  cascadeMC_GenerateChain.resolveDependency(&cascadeMC_DecayTable);
  cascadeMC_GenerateChain.resolveLoopManager(&cascadeMC_LoopManager);
  //cascadeMC_GenerateChain.reset_and_calculate();

  // Generate histogram for cascade MC
  cascadeMC_Histograms.setOption<int>("cMC_NhistBins", 600);
  // FIXME: Check dependence on histogram parameters
  cascadeMC_Histograms.resolveDependency(&cascadeMC_InitialState);
  cascadeMC_Histograms.resolveDependency(&cascadeMC_GenerateChain);
  cascadeMC_Histograms.resolveDependency(&TH_ProcessCatalog_WIMP);
  cascadeMC_Histograms.resolveDependency(&SimYieldTable_DarkSUSY);
  cascadeMC_Histograms.resolveDependency(&cascadeMC_FinalStates);
  cascadeMC_Histograms.resolveLoopManager(&cascadeMC_LoopManager);
  //cascadeMC_Histograms.reset_and_calculate();

  // Check convergence of cascade MC
  // FIXME: Test dynamic convergence criteria for cascade routines
  cascadeMC_EventCount.resolveDependency(&cascadeMC_InitialState);
  cascadeMC_EventCount.resolveLoopManager(&cascadeMC_LoopManager);
  //cascadeMC_EventCount.reset_and_calculate();

  // Start cascade MC loop

  // Infer gamma-ray spectra for recorded MC results
  cascadeMC_gammaSpectra.resolveDependency(&GA_missingFinalStates);
  cascadeMC_gammaSpectra.resolveDependency(&cascadeMC_FinalStates);
  cascadeMC_gammaSpectra.resolveDependency(&cascadeMC_Histograms);
  cascadeMC_gammaSpectra.resolveDependency(&cascadeMC_EventCount);

  // Calculate total gamma-ray yield (cascade MC + tabulated results)
  GA_AnnYield_General.resolveDependency(&TH_ProcessCatalog_WIMP);
  GA_AnnYield_General.resolveDependency(&SimYieldTable_DarkSUSY);
  GA_AnnYield_General.resolveDependency(&DarkMatter_ID_WIMP);
  GA_AnnYield_General.resolveDependency(&cascadeMC_gammaSpectra);

  // FIXME: Extend existing gamma-ray spectrum dumper
  dump_GammaSpectrum.resolveDependency(&GA_AnnYield_General);

  // Calculate Fermi LAT dwarf likelihood
  // FIXME: Check whether Fermi lat limits can be reproduced
  lnL_FermiLATdwarfs_gamLike.resolveDependency(&GA_AnnYield_General);
  lnL_FermiLATdwarfs_gamLike.resolveDependency(&RD_fraction_fixed);
  lnL_FermiLATdwarfs_gamLike.resolveBackendReq(&Backends::gamLike_1_0_0::Functown::lnL);


  // -- Calculate relic density --
  RD_eff_annrate_from_ProcessCatalog.notifyOfModel("SingletDM");
  RD_eff_annrate_from_ProcessCatalog.resolveDependency(&TH_ProcessCatalog_WIMP);
  RD_eff_annrate_from_ProcessCatalog.resolveDependency(&DarkMatter_ID_WIMP);

  RD_spectrum_from_ProcessCatalog.resolveDependency(&TH_ProcessCatalog_WIMP);
  RD_spectrum_from_ProcessCatalog.resolveDependency(&DarkMatter_ID_WIMP);

  RD_spectrum_ordered_func.resolveDependency(&RD_spectrum_from_ProcessCatalog);

  RD_oh2_general.resolveDependency(&RD_spectrum_ordered_func);
  RD_oh2_general.resolveDependency(&RD_eff_annrate_from_ProcessCatalog);
  RD_oh2_general.resolveBackendReq(&Backends::DarkSUSY_5_1_3::Functown::dsrdthlim);
  RD_oh2_general.resolveBackendReq(&Backends::DarkSUSY_5_1_3::Functown::dsrdtab);
  RD_oh2_general.resolveBackendReq(&Backends::DarkSUSY_5_1_3::Functown::dsrdeqn);
  RD_oh2_general.resolveBackendReq(&Backends::DarkSUSY_5_1_3::Functown::dsrdwintp);
  RD_oh2_general.resolveBackendReq(&Backends::DarkSUSY_5_1_3::Functown::widths);
  RD_oh2_general.resolveBackendReq(&Backends::DarkSUSY_5_1_3::Functown::rdmgev);
  RD_oh2_general.resolveBackendReq(&Backends::DarkSUSY_5_1_3::Functown::rdpth);
  RD_oh2_general.resolveBackendReq(&Backends::DarkSUSY_5_1_3::Functown::rdpars);
  RD_oh2_general.resolveBackendReq(&Backends::DarkSUSY_5_1_3::Functown::rdswitch);
  RD_oh2_general.resolveBackendReq(&Backends::DarkSUSY_5_1_3::Functown::rdlun);
  RD_oh2_general.resolveBackendReq(&Backends::DarkSUSY_5_1_3::Functown::rdpadd);
  RD_oh2_general.resolveBackendReq(&Backends::DarkSUSY_5_1_3::Functown::rddof);
  RD_oh2_general.resolveBackendReq(&Backends::DarkSUSY_5_1_3::Functown::rderrors);
  RD_oh2_general.resolveBackendReq(&Backends::DarkSUSY_5_1_3::Functown::DSparticle_code);


  // ---- Calculate direct detection constraints ----

  // Calculate direct detection rates for LUX 2013
  LUX_2013_Calc.resolveBackendReq(&Backends::DDCalc_1_0_0::Functown::DDCalc_Experiment);
  LUX_2013_Calc.resolveBackendReq(&Backends::DDCalc_1_0_0::Functown::DDCalc_CalcRates_simple);

  // Calculate direct detection likelihood for LUX 2013
  LUX_2013_GetLogLikelihood.resolveDependency(&LUX_2013_Calc);
  LUX_2013_GetLogLikelihood.resolveBackendReq(&Backends::DDCalc_1_0_0::Functown::DDCalc_Experiment);
  LUX_2013_GetLogLikelihood.resolveBackendReq(&Backends::DDCalc_1_0_0::Functown::DDCalc_LogLikelihood);

  // Set generic WIMP mass object
  mwimp_generic.resolveDependency(&TH_ProcessCatalog_WIMP);
  mwimp_generic.resolveDependency(&DarkMatter_ID_WIMP);
  sigma_SI_p_simple.resolveDependency(&DD_couplings_WIMP);
  sigma_SI_p_simple.resolveDependency(&mwimp_generic);

  // Spectral tests
  if ( (mode > 0) and (mode < 6) )
  {
    std::cout << "Producing test spectra." << std::endl;
    double mass = 100.;
    double sv = 3e-26;
    if (mode==1) dumpSpectrum("dNdE1.dat", mass, sv, Funk::vec<double>(0., 1., 0., 0., 0., 0.));
    if (mode==2) dumpSpectrum("dNdE2.dat", mass, sv, Funk::vec<double>(0., 0., 1., 0., 0., 0.));
    if (mode==3) dumpSpectrum("dNdE3.dat", mass, sv, Funk::vec<double>(0., 0., 0., 1., 0., 0.));
    if (mode==4) dumpSpectrum("dNdE4.dat", mass, sv, Funk::vec<double>(0., 0., 0., 0., 1., 0.));
    if (mode==5) dumpSpectrum("dNdE5.dat", mass, sv, Funk::vec<double>(0., 0., 0., 0., 0., 1.));
  }

  if (mode==6)
  {
    // Systematic parameter maps annihilation
    std::cout << "Producing test maps." << std::endl;
    int mBins = 40;
    int svBins = 20;
    std::vector<double> m_list = Funk::logspace(1.0, 3.0, mBins);
    std::vector<double> sv_list = Funk::logspace(-28.0, -24.0, svBins);
    boost::multi_array<double, 2> lnL_array{boost::extents[mBins][svBins]};
    boost::multi_array<double, 2> oh2_array{boost::extents[mBins][svBins]};
    for (size_t i = 0; i < m_list.size(); i++)
    {
      for (size_t j = 0; j < sv_list.size(); j++)
      {
        TH_ProcessCatalog_WIMP.setOption<double>("mWIMP", m_list[i]);
        TH_ProcessCatalog_WIMP.setOption<double>("sv", sv_list[j]);
        TH_ProcessCatalog_WIMP.setOption<std::vector<double>>("brList", Funk::vec<double>(1., 0., 0., 0., 0., 0.));
        //TH_ProcessCatalog_WIMP.setOption<std::vector<double>>("brList", Funk::vec<double>(0., 0., 1., 0., 0., 0.));
        std::cout << "Parameters: " << m_list[i] << " " << sv_list[j] << std::endl;
        DarkMatter_ID_WIMP.reset_and_calculate();
        TH_ProcessCatalog_WIMP.reset_and_calculate();
        RD_fraction_fixed.reset_and_calculate();
        SimYieldTable_DarkSUSY.reset_and_calculate();
        GA_missingFinalStates.reset_and_calculate();
        cascadeMC_FinalStates.reset_and_calculate();
        cascadeMC_DecayTable.reset_and_calculate();
        cascadeMC_LoopManager.reset_and_calculate();
        cascadeMC_gammaSpectra.reset_and_calculate();
        GA_AnnYield_General.reset_and_calculate();
        //dump_GammaSpectrum.reset_and_calculate();
        lnL_FermiLATdwarfs_gamLike.reset_and_calculate();
        double lnL = lnL_FermiLATdwarfs_gamLike(0);
        std::cout << "Fermi LAT likelihood: " << lnL << std::endl;
        lnL_array[i][j] = lnL;
        RD_eff_annrate_from_ProcessCatalog.reset_and_calculate();
        RD_spectrum_from_ProcessCatalog.reset_and_calculate();
        RD_spectrum_ordered_func.reset_and_calculate();
        RD_oh2_general.reset_and_calculate();
        double oh2 = RD_oh2_general(0);
        oh2_array[i][j] = oh2;
  //    LUX_2013_Calc.reset_and_calculate();
  //    LUX_2013_GetLogLikelihood.reset_and_calculate();
      }
    }
    dump_array_to_file("Fermi_table.dat", lnL_array, m_list, sv_list);
    dump_array_to_file("oh2_table.dat", oh2_array, m_list, sv_list);
  }

  if (mode==7)
  {
    // Systematic parameter maps scattering
    std::cout << "Producing test maps." << std::endl;
    int mBins = 40;
    int sBins = 40;
    std::vector<double> m_list = Funk::logspace(0.0, 4.0, mBins);
    std::vector<double> s_list = Funk::logspace(-10, -6, sBins);
    boost::multi_array<double, 2> lnL_array{boost::extents[mBins][sBins]};
    boost::multi_array<double, 2> oh2_array{boost::extents[mBins][sBins]};
    TH_ProcessCatalog_WIMP.setOption<double>("sv", 0.);
    TH_ProcessCatalog_WIMP.setOption<std::vector<double>>("brList", Funk::vec<double>(1., 0., 0., 0., 0., 0.));
    for (size_t i = 0; i < m_list.size(); i++)
    {
      for (size_t j = 0; j < s_list.size(); j++)
      {
        TH_ProcessCatalog_WIMP.setOption<double>("mWIMP", m_list[i]);
        std::cout << "Parameters: " << m_list[i] << " " << s_list[j] << std::endl;
        DarkMatter_ID_WIMP.reset_and_calculate();
        TH_ProcessCatalog_WIMP.reset_and_calculate();
        RD_fraction_fixed.reset_and_calculate();
        DD_couplings_WIMP.setOption<double>("gps", s_list[j]);
        DD_couplings_WIMP.setOption<double>("gns", 0.);
        DD_couplings_WIMP.setOption<double>("gpa", 0.);
        DD_couplings_WIMP.setOption<double>("gna", 0.);
        DD_couplings_WIMP.reset_and_calculate();
        mwimp_generic.reset_and_calculate();
        sigma_SI_p_simple.reset_and_calculate();
        double sigma_SI_p = sigma_SI_p_simple(0);
        std::cout << "sigma_SI_p: " << sigma_SI_p << std::endl;
        DDCalc_1_0_0_init.reset_and_calculate();
        LUX_2013_Calc.reset_and_calculate();
        LUX_2013_GetLogLikelihood.reset_and_calculate();
        double lnL = LUX_2013_GetLogLikelihood(0);
        std::cout << "LUX2013 lnL = " << lnL << std::endl;
        lnL_array[i][j] = lnL;
      }
    }
    dump_array_to_file("LUX2013_table.dat", lnL_array, m_list, s_list);
  }

  std::cout << "Done!" << std::endl;
  return 0;
}