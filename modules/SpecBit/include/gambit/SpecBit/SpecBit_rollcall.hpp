//   GAMBIT: Global and Modular BSM Inference Tool
//   *********************************************
///  \file
///
///  Rollcall header for module SpecBit
///
///  These functions link ModelParameters to 
///  Spectrum objects in various ways.
///
///  *********************************************
///
///  Authors (add name and date if you modify):
///
///  \author Ben Farmer
///          (ben.farmer@gmail.com)
///    \date 2014 Sep - Dec, 2015 Jan
///  
///  *********************************************

#ifndef __SpecBit_rollcall_hpp__
#define __SpecBit_rollcall_hpp__

#define MODULE SpecBit
START_MODULE

  // Functions to supply particle spectra in various forms
  
  // This capability supplies the physical mass spectrum of the MSSM plus running
  // parameters in the DRbar scheme. This can be generated by many different 
  // constrained models with various boundary conditions, or defined directly.
  #define CAPABILITY MSSM_spectrum   
  START_CAPABILITY                          

    // ==========================
    // GUT MSSM parameterisations 
    // (CMSSM and its various non-universal generalisations)    
 
    /// Get MSSM spectrum from CMSSM boundary conditions
    //  The type, SMplusUV, is a struct containing two Spectrum* members and an SMInputs
    //  member. The Spectrum* members point to a "UV" Spectrum object (the MSSM) and an
    //  "SM" Spectrum object (an effective Standard Model description), while SMInputs
    //  contains the information in the SMINPUTS block defined by SLHA2.
    #define FUNCTION get_CMSSM_spectrum            
    START_FUNCTION(SMplusUV)                  
    ALLOW_MODELS(CMSSM)
    #undef FUNCTION

    // FlexibleSUSY compatible maximal CMSSM generalisation (MSSM with GUT boundary conditions) 
    #define FUNCTION get_MSSMatMGUT_spectrum
    START_FUNCTION(SMplusUV)                  
    ALLOW_MODELS(MSSM78atMGUT)
    #undef FUNCTION

    // ============================== 
    // MSSM parameterised with input at (user-defined) scale Q 
    #define FUNCTION get_MSSMatQ_spectrum
    START_FUNCTION(SMplusUV)                  
    ALLOW_MODELS(MSSM78atQ)
    #undef FUNCTION

    // ============================== 

    // Extract appropriate Spectrum* from SMplusUV struct, while preserving the Capability
    #define FUNCTION get_MSSM_spectrum_as_SpectrumPtr
    START_FUNCTION(CSpectrum*)  // Note; CSpectrum* = const Spectrum*
    DEPENDENCY(MSSM_spectrum, SMplusUV)
    #undef FUNCTION


    // Get MSSM spectrum as an SLHAea object
    #define FUNCTION get_MSSM_spectrum_as_SLHAea
    START_FUNCTION(eaSLHA)                  
    DEPENDENCY(MSSM_spectrum, CSpectrum*)           // Takes a (pointer to a) Spectrum object and returns an eaSLHA object
    #undef FUNCTION

  #undef CAPABILITY

  // For testing only
  #define CAPABILITY test_MSSM_spectrum   
  START_CAPABILITY                          
    #define FUNCTION make_test_spectrum  // Get (pointer to) test MSSM spectrum
    START_FUNCTION(Spectrum*)
    #undef FUNCTION
  #undef CAPABILITY

  // Functions to changes the capability associated with a Spectrum object to "SM_spectrum"
  ///TODO: CURRENTLY THERE SEEMS TO BE A BUG WITH RETRIEVING THESE DEPENDENCIES! SWITCHING BACK TO OLD METHOD
  //QUICK_FUNCTION(SpecBit, SM_spectrum, NEW_CAPABILITY, convert_MSSM_to_SM,   Spectrum*, (), (MSSM_spectrum, Spectrum*))
  //QUICK_FUNCTION(MODULE, SM_spectrum, OLD_CAPABILITY, convert_NMSSM_to_SM,  Spectrum*, (), (NMSSM_spectrum, Spectrum*))
  //QUICK_FUNCTION(MODULE, SM_spectrum, OLD_CAPABILITY, convert_E6MSSM_to_SM, Spectrum*, (), (E6MSSM_spectrum, Spectrum*))

  // Note: QUICK_FUNCTION usage:
  // Arguments: MODULE, CAPABILITY, NEW_CAPABILITY_FLAG, FUNCTION, TYPE, (n x ALLOWED_MODEL), m x (DEPENDENCY, DEPENDENCY_TYPE)
  //            The last two arguments are optional, and n and m can be anything from 0 to 10.
  //
  // equivalent to:
  #define CAPABILITY SM_spectrum
  START_CAPABILITY                          
    #define FUNCTION convert_MSSM_to_SM
    START_FUNCTION(CSpectrum*)
    DEPENDENCY(MSSM_spectrum, CSpectrum*)
    #undef FUNCTION 

    // etc. for other functions

  #undef CAPABILITY


  // 'Convenience' functions to retrieve certain particle properities in a simple format

  // #define CAPABILITY LSP_mass   // Supplies the mass of the lightest supersymmetric particle
  // START_CAPABILITY

  //   #define FUNCTION get_LSP_mass                      // Retrieves the LSP mass from a Spectrum object
  //   START_FUNCTION(double)                  
  //   DEPENDENCY(particle_spectrum, SpectrumPtr)            // Takes a Spectrum object and returns an eaSLHA object
  //   ALLOW_MODELS(MSSM24, CMSSM)
  //   #undef FUNCTION

  // #undef CAPABILITY
    

  /// TEST FUNCTIONS
  // Just some functions for testing SpecBit and Spectrum object components

  #define CAPABILITY specbit_tests1
  START_CAPABILITY
     #define FUNCTION specbit_test_func1
     START_FUNCTION(double)
     DEPENDENCY(MSSM_spectrum, CSpectrum*)
     #undef FUNCTION
  #undef CAPABILITY

  #define CAPABILITY specbit_tests2
  START_CAPABILITY
     #define FUNCTION specbit_test_func2
     START_FUNCTION(double)
     #undef FUNCTION
  #undef CAPABILITY

  #define CAPABILITY specbit_tests3
  START_CAPABILITY
     #define FUNCTION specbit_test_func3
     START_FUNCTION(double)
     DEPENDENCY(SM_spectrum, CSpectrum*)
     #undef FUNCTION
  #undef CAPABILITY

  #define CAPABILITY specbit_test_SMplusUV
  START_CAPABILITY
     #define FUNCTION specbit_test_SMplusUV
     START_FUNCTION(double)
     DEPENDENCY(MSSM_spectrum, SMplusUV)
     #undef FUNCTION
  #undef CAPABILITY


 #define CAPABILITY dump_spectrum_slha
 START_CAPABILITY
 
     #define FUNCTION dump_spectrum
     START_FUNCTION(double)
     DEPENDENCY(SM_spectrum, CSpectrum*)
     #undef FUNCTION

 #undef CAPABILITY


 #define CAPABILITY SpecBit_examples
 START_CAPABILITY

     #define FUNCTION exampleRead
     START_FUNCTION(bool)
     DEPENDENCY(MSSM_spectrum, CSpectrum*)
     #undef FUNCTION

 #undef CAPABILITY

#undef MODULE

#endif /* defined(__SpecBit_rollcall_hpp__) */



