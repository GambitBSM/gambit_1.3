//   GAMBIT: Global and Modular BSM Inference Tool
//   *********************************************
///  \file
///
///  Implementation of scalar singlet DM routines.
///
///  *********************************************
///
///  Authors (add name and date if you modify):
///   
///  \author Christoph Weniger  
///          (c.weniger@uva.nl)
///  \date Oct 2014, Apr 2015
///
///  \author Torsten Bringmann 
///  \date May 2015
///
///  *********************************************

#include "gambit/Elements/gambit_module_headers.hpp"
#include "gambit/DarkBit/DarkBit_rollcall.hpp"
#include "gambit/Utils/ASCIItableReader.hpp"
#include "boost/make_shared.hpp"

namespace Gambit {
  namespace DarkBit {

    class SingletDM
    {
      public:
        /// Initialize SingletDM object (branching ratios etc)
        SingletDM(
            TH_ProcessCatalog &catalog,
            std::map<std::string, Funk::Funk> & arg_f_vs_mass)
        {
          mh   = catalog.particleProperties.at("h0_1").mass;
          // FIXME: This should not be hard-coded
          v0   = 246.0;
          alpha_s = 0.12;
          mb   = catalog.particleProperties.at("d_3").mass;
          mc   = catalog.particleProperties.at("u_2").mass;
          mtau = catalog.particleProperties.at("tau-").mass;
          mt   = catalog.particleProperties.at("u_3").mass;

          f_vs_mass = arg_f_vs_mass;
          Gamma = f_vs_mass["Gamma"]->bind("mass");
          Gamma_mh = Gamma->eval(mh);
        };
        ~SingletDM() {}

        /// Helper function (Breit-Wigner)
        double Dh2 (double s)
        {
          return 1/((s-mh*mh)*(s-mh*mh)+mh*mh*Gamma_mh*Gamma_mh);
        }

        /*! \brief Returns <sigma v> in cm3/s for given channel, velocity and
         *         model parameters.
         *
         * channel: bb, tautau, mumu, ss, cc, tt, gg, gammagamma, Zgamma, WW,
         * ZZ, hh
         */
        double sv(std::string channel, double lambda, double mass, double v)
        {
          // FIXME: Comment this in to get a segfault... Sigh
          //f_vs_mass["Gamma"]->bind("mass");
          // FIXME: Comment this in to get not a segfault... Sigh-sigh
          //auto h = f_vs_mass["Gamma"]->bind("mass");
          // I think this is related to a bug in the funktions.hpp.  But I
          // don't understand how.
          double s = 4*mass*mass/(1-v*v/4);
          double sqrt_s = sqrt(s);
          if ( sqrt_s < 90 ) 
          {
            // FIXME: This should not crash the code
            invalid_point().raise(
                "SingletDM sigmav called with sqrt_s < 90 GeV.");
            return 0;
          }

          if ( channel == "hh" ) { return sv_hh(lambda, mass, v); }
          if ( sqrt_s < 300 )
          {
            double br = f_vs_mass[channel]->bind("mass")->eval(sqrt_s);
            double Gamma_s = Gamma->eval(sqrt_s);
            double GeV2tocm3s1 = gev2cm2*s2cm;

            double res = 2*lambda*lambda*v0*v0/
              sqrt_s*Dh2(s)*Gamma_s*GeV2tocm3s1*br;
            return res;
          }
          else
          {
            if ( channel == "bb" and sqrt_s > mb )
              return sv_ff(lambda, mass, v, mb, true);
            if ( channel == "cc" and sqrt_s > mc  )
              return sv_ff(lambda, mass, v, mc, false);
            if ( channel == "tautau" and sqrt_s > mtau )
              return sv_ff(lambda, mass, v, mtau, false);
            if ( channel == "tt" and sqrt_s > mt )
              return sv_ff(lambda, mass, v, mt, false);
            if ( channel == "ZZ" and sqrt_s > m_Zboson)
              return sv_ZZ(lambda, mass, v);
            if ( channel == "WW" and sqrt_s > m_Wboson)
              return sv_WW(lambda, mass, v);
          }
          return 0;
        }

        // Annihilation into W bosons.
        double sv_WW(double lambda, double mass, double v)
        {
          double s = 4*mass*mass/(1-v*v/4);
          double x = pow(m_Wboson,2)/s;
          double GeV2tocm3s1 = gev2cm2*s2cm;
          return pow(lambda,2)*s/8/M_PI*sqrt(1-4*x)*Dh2(s)*(1-4*x+12*pow(x,2))
            *GeV2tocm3s1;
        }

        // Annihilation into Z bosons.
        double sv_ZZ(double lambda, double mass, double v)
        {
          double s = 4*mass*mass/(1-v*v/4);
          double x = pow(m_Zboson,2)/s;
          double GeV2tocm3s1 = gev2cm2*s2cm;
          return pow(lambda,2)*s/16/M_PI*sqrt(1-4*x)*Dh2(s)*(1-4*x+12*pow(x,2))
            * GeV2tocm3s1;
        }

        // Annihilation into fermions
        double sv_ff(
            double lambda, double mass, double v, double mf, bool is_quark)
        {
          double s = 4*mass*mass/(1-v*v/4);
          double vf = sqrt(1-4*pow(mf,2)/s);
          double Xf = 1;
          if ( is_quark ) Xf = 3 * 
            (1+(3/2*log(pow(mf,2)/s)+9/4)*4*alpha_s/3/M_PI);
          double GeV2tocm3s1 = gev2cm2*s2cm;
          return pow(lambda,2)*
            pow(mf,2)/4/M_PI*Xf*pow(vf,3) * Dh2(s) * GeV2tocm3s1;
        }

        /// Calculate <sigma v> for hh final states
        double sv_hh(double lambda, double mass, double v)
        {
          double s = 4*mass*mass/(1-v*v/4);  // v is relative velocity
          double vh = sqrt(1-4*mh*mh/s);  // vh and vs are lab velocities
          // Hardcoded lower velocity avoids nan results
          double vs = std::max(v/2, 1e-6);  
          double tp = pow(mass,2)+pow(mh,2)-0.5*s*(1-vs*vh);
          double tm = pow(mass,2)+pow(mh,2)-0.5*s*(1+vs*vh);

          double aR = 1+3*mh*mh*(s-mh*mh)*Dh2(s);
          double aI = 3*mh*mh*sqrt(s)*Gamma_mh*Dh2(s);

          return pow(lambda,2)/16/M_PI/pow(s,2)/vs *
            (
             (pow(aR,2)+pow(aI,2))*s*vh*vs
             +4*lambda*pow(v0,2)*(aR-lambda*pow(v0,2)/(s-2*pow(mh,2)))
             *log(abs(pow(mass,2)-tp)/abs(pow(mass,2)-tm))
             +(2*pow(lambda,2)*pow(v0,4)*s*vh*vs)
             /(pow(mass,2)-tm)/(pow(mass,2)-tp));
        }

      private:
        std::map<std::string, Funk::Funk> f_vs_mass;
        Funk::BoundFunk Gamma;

        double Gamma_mh, mh, v0, alpha_s, mb, mc, mtau, mt;
    };

    void DarkMatter_ID_SingletDM(DarkMatter_ID_type & result)
    {
      using namespace Pipes::DarkMatter_ID_SingletDM;
      result = DarkMatter_ID_type(initVector<std::string>("S"));
    } // DarkMatter_ID_SingletDM

// Obsolete !
//    /// Initializes thresholds/resonances for RD calculation for SingletDM
//    void RD_thresholds_resonances_SingletDM(TH_resonances_thresholds &result)
//    {
//      using namespace Pipes::RD_thresholds_resonances_SingletDM;
//
//      result.resonances.clear();
//      result.threshold_energy.clear();
//
//      result.threshold_energy.push_back(2*(*Param["mass"]));
//      double mh = (*Dep::TH_ProcessCatalog).getParticleProperty("h0_1").mass;
//      result.resonances.push_back(TH_Resonance(mh, 0.01));  // FIXME use proper Higgs width!
//
//      // FIXME: Move somewhere else
//      DS_RDMGEV myrdmgev;
//      myrdmgev.nco = 1;
//      myrdmgev.mco(1) = *Param["mass"];
//      myrdmgev.mdof(1) = 1;
//      myrdmgev.kcoann(1) = 42;  // ???
//      *BEreq::rdmgev = myrdmgev;
//    } // function RD_thresholds_resonances_SingletDM


// OBSOLETE
//   /*! \brief Collects information about coannihilating particles, resonances and
//     * threshold energies.
//     */
//    void RD_spectrum_SingletDM(RD_spectrum_type &result)
//    {
//      using namespace Pipes::RD_spectrum_SingletDM;
//
//      result.coannihilatingParticles.clear();
//      // add WIMP=least massive 'coannihilating particle'
//      // NB: particle code (1st entry) is irrelevant (unless Weff is ibatined from DS)
//      result.coannihilatingParticles.push_back(
//          RD_coannihilating_particle(100,1,*Param["mass"]));
//
//      // now derive thresholds & resonances from process catalogue
//      std::string DMid= Dep::DarkMatter_ID->singleID();
//      TH_Process annihilation = 
//              (*Dep::TH_ProcessCatalog).getProcess(DMid, DMid);
//      result.resonances = annihilation.thresholdResonances.resonances;
//      result.threshold_energy = annihilation.thresholdResonances.threshold_energy;
//
//      // coannihilation thresholds would have to be added now -- but don't exist for SingletDM...
//
//    } // function RD_spectrum_SingletDM




    /// Direct detection couplings for Singlet DM.
    void DD_couplings_SingletDM(Gambit::DarkBit::DD_couplings &result)
    {
      using namespace Pipes::DD_couplings_SingletDM;
      double mass = *Param["mass"];
      double lambda = *Param["lambda"];
      // FIXME: It would be cleaner if this actually does not come from the
      // process catalog, but instead from the Spectrum Object
      double mh = (*Dep::TH_ProcessCatalog).getParticleProperty("h0_1").mass;

      // TODO: Double check expressions (taken from Cline et al. 2013)
      double fp = 2./9. + 7./9.*(*Param["fpu"] + *Param["fpd"] + *Param["fps"]);
      double fn = 2./9. + 7./9.*(*Param["fnu"] + *Param["fnd"] + *Param["fns"]);

      result.gps = lambda*fp*m_neutron/pow(mh,2)/mass/2;
      result.gns = lambda*fn*m_proton/pow(mh,2)/mass/2;
      result.gpa = 0;  // Only SI cross-section
      result.gna = 0;
      result.M_DM = *Param["mass"];

      logger() << "Singlet DM DD couplings:" << std::endl;
      logger() << " gps = " << result.gps << std::endl;
      logger() << " gns = " << result.gns << std::endl;
      logger() << " gpa = " << result.gpa << std::endl;
      logger() << " gna = " << result.gna << std::endl;
      logger() << "M_DM = " << result.M_DM << std::endl;

    } // function DD_couplings_SingletDM

    std::map<std::string, Funk::Funk> get_f_vs_mass(std::string filename)
    {
      // Higgs branching ratios and total width Gamma [GeV], as function of
      // mass [GeV] (90 - 150 GeV)
      ASCIItableReader table(filename);
      std::vector<std::string> colnames = 
        initVector<std::string>("mass", "bb", "tautau", "mumu",
            "ss", "cc", "tt", "gg", "gammagamma", "Zgamma",
            "WW", "ZZ", "Gamma");
      table.setcolnames(colnames);

      std::map<std::string, Funk::Funk> f_vs_mass;
      for (auto it = colnames.begin(); it != colnames.end(); it++)
      {
        f_vs_mass[*it] = Funk::interp("mass", table["mass"], table[*it]);
      }
      return f_vs_mass;
    }

    /// Set up process catalogue for Singlet DM.
    void TH_ProcessCatalog_SingletDM(Gambit::DarkBit::TH_ProcessCatalog &result)
    {
      using namespace Pipes::TH_ProcessCatalog_SingletDM;
      using std::vector;
      using std::string;

      double mass = *Param["mass"];
      double lambda = *Param["lambda"];

      // Initialize catalog
      TH_ProcessCatalog catalog;
      TH_Process process_ann((std::string)"S", (std::string)"S");

      ///////////////////////////
      // Import particle masses
      ///////////////////////////
      
      // Import based on Spectrum objects
      const SubSpectrum* SM = *Dep::SM_spectrum;
      const SMInputs& SMI   = *Dep::SMINPUTS;  

      // Get SM masses
#define getSMmass(Name, spinX2)                                                \
      catalog.particleProperties.insert(                                       \
          std::pair<std::string, TH_ParticleProperty>(                         \
            Name , TH_ParticleProperty(SM->phys.get_Pole_Mass(Name), spinX2)   \
            )                                                                  \
          );    
      getSMmass("e-",     1)
      getSMmass("e+",     1)
      getSMmass("mu-",    1)
      getSMmass("mu+",    1)
      getSMmass("tau-",   1)
      getSMmass("tau+",   1)
      getSMmass("nu_1",   1)
      getSMmass("nubar_1",1) 
      getSMmass("nu_2",   1)
      getSMmass("nubar_2",1) 
      getSMmass("nu_3",   1)
      getSMmass("nubar_3",1)      
      getSMmass("Z0",     2)
      getSMmass("W+",     2)
      getSMmass("W-",     2)      
      getSMmass("g",      2)   
      getSMmass("gamma",  2)   
      getSMmass("b",      1)
      getSMmass("bbar",   1)
      getSMmass("t",      1)
      getSMmass("tbar",   1)
#undef getSMmass
#define getSMmassMS(Name, Mass, spinX2)                                        \
      catalog.particleProperties.insert(                                       \
          std::pair<std::string, TH_ParticleProperty>(                         \
            Name , TH_ParticleProperty(Mass, spinX2)                           \
            )                                                                  \
          );    
      // Pole masses not available for the light quarks.    
      getSMmassMS("d"   , SMI.mD,  1) // md(2 GeV)^MS-bar, not pole mass
      getSMmassMS("dbar", SMI.mD,  1) // md(2 GeV)^MS-bar, not pole mass
      getSMmassMS("u"   , SMI.mU,  1) // mu(2 GeV)^MS-bar, not pole mass
      getSMmassMS("ubar", SMI.mU,  1) // mu(2 GeV)^MS-bar, not pole mass
      getSMmassMS("s"   , SMI.mS,  1) // ms(2 GeV)^MS-bar, not pole mass
      getSMmassMS("sbar", SMI.mS,  1) // ms(2 GeV)^MS-bar, not pole mass
      getSMmassMS("c"   , SMI.mCmC,1) // mc(mc)^MS-bar, not pole mass
      getSMmassMS("cbar", SMI.mCmC,1) // mc(mc)^MS-bar, not pole mass
      // Dummy masses for neutrino flavour eigenstates. Set to zero.
      getSMmassMS("nu_e",     0.0, 1)
      getSMmassMS("nubar_e",  0.0, 1)
      getSMmassMS("nu_mu",    0.0, 1)
      getSMmassMS("nubar_mu", 0.0, 1)
      getSMmassMS("nu_tau",   0.0, 1)
      getSMmassMS("nubar_tau",0.0, 1)      
#undef getSMmassMS  

      // Insert singlet mass
      TH_ParticleProperty S_Property(mass, 1);
      catalog.particleProperties.insert(
          std::pair<std::string, TH_ParticleProperty> ("S", S_Property));
      
      // FIXME: Get Higgs mass from Spec Bit
      TH_ParticleProperty h0_1_Property(125.7, 0);
      catalog.particleProperties.insert(
          std::pair<std::string, TH_ParticleProperty> ("h0_1", h0_1_Property));

      // Instantiate tables (only once)
      static std::map<std::string, Funk::Funk> f_vs_mass = 
        get_f_vs_mass("DarkBit/data/Higgs_decay_1101.0593.dat");

      /////////////////////////////
      // Import Decay information
      /////////////////////////////

      // Import based on decay table from DecayBit
      const DecayTable* tbl = &(*Dep::decay_rates);
      
      // List of decays to include
      const vector<string> decaysOfInterest = initVector<string> ("h0_1");        
      
      double minBranching = runOptions->getValueOrDef<double>(0.0,
          "ProcessCatalog_MinBranching");
      for(auto iState_it = decaysOfInterest.begin();
          iState_it != decaysOfInterest.end(); ++iState_it)
      {
        std::cout << 
          "Importing decay information for: " << *iState_it << std::endl;
        const DecayTable::Entry &entry = tbl->at(*iState_it);
        double totalWidth = entry.width_in_GeV;
        if(totalWidth>0)
        {     
          TH_Process process(*iState_it);
          process.genRateTotal = Funk::cnst(totalWidth);
          for(auto fState_it = entry.channels.begin();
              fState_it!= entry.channels.end(); ++fState_it)
          {
            double bFraction = (fState_it->second).first;
            if(bFraction>minBranching)
            {
              vector<string> pIDs;
              std::cout << "- ";
              const double m_init  = catalog.getParticleProperty(*iState_it).mass;
              double m_final = 0;
              for(auto pit = fState_it->first.begin();
                  pit != fState_it->first.end(); ++pit)
              {
                std::string name = Models::ParticleDB().long_name(*pit);
                m_final += catalog.getParticleProperty(name).mass;
                pIDs.push_back(name);
                std::cout << name << "\t";
              } 
              double partialWidth = totalWidth * bFraction;       
              bool checkKinematics = runOptions->getValueOrDef<bool>(true,
              "ProcessCatalog_KinCheck");
              // TODO: Add other criteria on which channels to include?
              if(!checkKinematics or m_final<=m_init)
              {
                std::cout<< bFraction << std::endl;   
                process.channelList.push_back(
                    TH_Channel(pIDs, Funk::cnst(partialWidth)));
              }
              else
                std::cout<< "kin. closed" << std::endl;   
            }
          }
          catalog.processList.push_back(process);
        }
      }       

      // Instantiate SingletDM object
      auto singletDM = boost::make_shared<SingletDM>(catalog, f_vs_mass);

      // FIXME: Add top (is this still TODO?)

      // Populate channel list and add thresholds
      // FIXME: Mass eigenstates are now being used here. Check if CKM factors are necessary anywhere.
      // lowest threshold = 2*WIMP rest mass  (unlike DS convention!)
      process_ann.thresholdResonances.threshold_energy.push_back(2*mass); 
      auto channel = Funk::vec<std::string>(
          "bb", "WW", "cc", "tautau", "ZZ", "tt", "hh");
      auto p1 = Funk::vec<std::string>("b",   "W+", "c",   "tau+", "Z0", "t",   "h0_1");
      auto p2 = Funk::vec<std::string>("bbar","W-", "cbar","tau-", "Z0", "tbar","h0_1");
      {
        for ( int i = 0; i < 7; i++ )
        {
          if ( mass > catalog.particleProperties.at(p1[i]).mass )
          {
            Funk::Funk kinematicFunction = 
              Funk::funcM(singletDM, &SingletDM::sv, channel[i], lambda,
                  mass, Funk::var("v"));
            std::vector<std::string> finalStates = Funk::vec<std::string>(p1[i], p2[i]);
            TH_Channel channel(finalStates, kinematicFunction);
            process_ann.channelList.push_back(channel);
          }
          else
          {
            process_ann.thresholdResonances.threshold_energy.push_back(2*catalog.particleProperties.at(p1[i]).mass); 
          }
        }
      }

      // Populate resonance list; Singlet model: only SM Higgs can appear as resonance 
      double resmasses[] = {catalog.getParticleProperty("h0_1").mass};
      double reswidths[] = {0.01};  // FIXME: import Higgs width
      int resmax=sizeof(resmasses) / sizeof(resmasses[0]);

      for (int i=0; i<resmax; i++)
      {
        if (resmasses[i]/mass > 2.)
        {
          process_ann.thresholdResonances.resonances.push_back(TH_Resonance(resmasses[i], reswidths[i]));
        }
      }
      

      catalog.processList.push_back(process_ann);


      result = catalog;
    } // function TH_ProcessCatalog_SingletDM
  }
}
