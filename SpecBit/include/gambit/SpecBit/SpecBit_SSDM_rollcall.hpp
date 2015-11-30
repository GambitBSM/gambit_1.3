//   GAMBIT: Global and Modular BSM Inference Tool
//   *********************************************
///  \file
///
///  Rollcall declarations for module functions
///  contained in SpecBit_MSSM.cpp
///
///  *********************************************
///
///  Authors (add name and date if you modify):
///
///  \author Ben Farmer
///          (benjamin.farmer@fysik.su.se)
///    \date 2014 Sep - Dec, 2015 Jan - Mar
///  
///  \author Christopher Rogan
///          (christophersrogan@gmail.com)
///  \date 2015 Apr
///
///  *********************************************

#ifndef __SpecBit_SSDM_hpp__
#define __SpecBit_SSDM_hpp__


  #define CAPABILITY SSDM_spectrum
  START_CAPABILITY                          

    #define FUNCTION get_SSDM_spectrum
    START_FUNCTION(const Spectrum*)
    DEPENDENCY(SMINPUTS, SMInputs)
    ALLOW_MODEL_DEPENDENCE(StandardModel_Higgs_running, SingletDM_running)
    MODEL_GROUP(higgs,   (StandardModel_Higgs_running))
    MODEL_GROUP(singlet, (SingletDM_running))
    ALLOW_MODEL_COMBINATION(higgs, singlet)
    #undef FUNCTION

    #define FUNCTION check_perturb
    START_FUNCTION(bool)
    DEPENDENCY(SMINPUTS, SMInputs)
    DEPENDENCY(SSDM_spectrum,const Spectrum*)
    DEPENDENCY(vacuum_stability, ddpair)
    ALLOW_MODEL_DEPENDENCE(StandardModel_Higgs_running, SingletDM_running)
    MODEL_GROUP(higgs,   (StandardModel_Higgs_running))
    MODEL_GROUP(singlet, (SingletDM_running))
    ALLOW_MODEL_COMBINATION(higgs, singlet)
    #undef FUNCTION

  #undef CAPABILITY


  #define CAPABILITY vacuum_stability
  START_CAPABILITY                          

    #define FUNCTION find_min_lambda
    START_FUNCTION(ddpair)
    DEPENDENCY(SMINPUTS, SMInputs)
    DEPENDENCY(SSDM_spectrum,const Spectrum*)
    ALLOW_MODEL_DEPENDENCE(StandardModel_Higgs_running, SingletDM_running)
    MODEL_GROUP(higgs,   (StandardModel_Higgs_running))
    MODEL_GROUP(singlet, (SingletDM_running))
    ALLOW_MODEL_COMBINATION(higgs, singlet)
    #undef FUNCTION




    #define FUNCTION default_scale  // use this as the dependency for check_perturb without having to calculate minimum lambda
    START_FUNCTION(ddpair)
    ALLOW_MODEL_DEPENDENCE(StandardModel_Higgs_running, SingletDM_running)
    MODEL_GROUP(higgs,   (StandardModel_Higgs_running))
    MODEL_GROUP(singlet, (SingletDM_running))
    ALLOW_MODEL_COMBINATION(higgs, singlet)
    #undef FUNCTION


  #undef CAPABILITY


  #define CAPABILITY VS_likelihood
  START_CAPABILITY                          

    #define FUNCTION get_likelihood
    START_FUNCTION(double)
    DEPENDENCY(vacuum_stability, ddpair)
    ALLOW_MODEL_DEPENDENCE(StandardModel_Higgs_running, SingletDM_running)
    MODEL_GROUP(higgs,   (StandardModel_Higgs_running))
    MODEL_GROUP(singlet, (SingletDM_running))
    ALLOW_MODEL_COMBINATION(higgs, singlet)
    #undef FUNCTION

  #undef CAPABILITY


  #define CAPABILITY expected_lifetime
    START_CAPABILITY
    #define FUNCTION get_expected_lifetime
    START_FUNCTION(double)
    DEPENDENCY(vacuum_stability, ddpair)
    ALLOW_MODEL_DEPENDENCE(StandardModel_Higgs_running, SingletDM_running)
    MODEL_GROUP(higgs,   (StandardModel_Higgs_running))
    MODEL_GROUP(singlet, (SingletDM_running))
    ALLOW_MODEL_COMBINATION(higgs, singlet)
    #undef FUNCTION
  #undef CAPABILITY




  /// @}

#endif

