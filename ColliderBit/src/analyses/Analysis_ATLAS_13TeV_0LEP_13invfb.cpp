// -*- C++ -*-
#include "gambit/ColliderBit/analyses/BaseAnalysis.hpp"
#include "gambit/ColliderBit/ATLASEfficiencies.hpp"
#include "Eigen/Eigen"

namespace Gambit {
  namespace ColliderBit {

    using namespace std;
    using namespace HEPUtils;


    /// @brief ATLAS Run 2 0-lepton jet+MET SUSY analysis, with 13/fb of data
    ///
    /// Based on:
    ///   https://cds.cern.ch/record/2206252
    ///   https://atlas.web.cern.ch/Atlas/GROUPS/PHYSICS/CONFNOTES/ATLAS-CONF-2016-078/
    ///
    /// Recursive jigsaw reconstruction signal regions are currently not included
    ///
    class Analysis_ATLAS_13TeV_0LEP_13invfb : public HEPUtilsAnalysis {
    public:

      // Numbers passing cuts
      static const size_t NUMSR = 13;
      double _srnums[13] = {0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.};


      // vector<int> cutFlowVector;
      // vector<string> cutFlowVector_str;
      // size_t NCUTS; //=16;


      Analysis_ATLAS_13TeV_0LEP_13invfb() {
        set_luminosity(13.3);
        // NCUTS=60;
        // for (size_t i=0;i<NCUTS;i++){
        //   cutFlowVector.push_back(0);
        //   cutFlowVector_str.push_back("");
        // }
      }


      void analyze(const Event* event) {

        HEPUtilsAnalysis::analyze(event);


        // Missing energy
        const P4 pmiss = event->missingmom();
        const double met = event->met();


        // Get baseline jets
        /// @todo Drop b-tag if pT < 50 GeV or |eta| > 2.5?
        vector<const Jet*> baselineJets;
        for (const Jet* jet : event->jets())
          if (jet->pT() > 20. && jet->abseta() < 2.8) {
            baselineJets.push_back(jet);
          }

        // Get baseline electrons
        vector<const Particle*> baselineElectrons;
        for (const Particle* electron : event->electrons())
          if (electron->pT() > 10. && electron->abseta() < 2.47)
            baselineElectrons.push_back(electron);

        // Get baseline muons
        vector<const Particle*> baselineMuons;
        for (const Particle* muon : event->muons())
          if (muon->pT() > 10. && muon->abseta() < 2.7)
            baselineMuons.push_back(muon);


        // Remove any |eta| < 0.2 jet within dR = 0.2 of an electron
        /// @todo Unless b-tagged (and pT > 50 && abseta < 2.5)
        vector<const Jet*> signalJets;
        for (const Jet* j : baselineJets)
          if (j->abseta() > 2.8 ||
              all_of(baselineElectrons.begin(), baselineElectrons.end(),
                     [&](const Particle* e){ return deltaR_rap(*e, *j) > 0.2; }))
            signalJets.push_back(j);

        // Remove electrons with dR = 0.4 of surviving |eta| < 2.8 jets
        /// @todo Actually only within 0.2--0.4
        vector<const Particle*> signalElectrons;
        for (const Particle* e : baselineElectrons)
          if (all_of(signalJets.begin(), signalJets.end(),
                     [&](const Jet* j){ return j->abseta() > 2.8 || deltaR_rap(*e, *j) > 0.4; }))
            signalElectrons.push_back(e);
        // Apply electron ID selection
        /// @todo Use *loose* electron selection
        ATLAS::applyMediumIDElectronSelection(signalElectrons);

        // Remove muons with dR = 0.4 of surviving |eta| < 2.8 jets
        /// @todo Note says that dR is in rap rather than eta
        /// @todo Actually only within 0.2--0.4
        /// @todo Within 0.2, discard the *jet* based on jet track vs. muon criteria
        vector<const Particle*> signalMuons;
        for (const Particle* m : baselineMuons)
          if (all_of(signalJets.begin(), signalJets.end(),
                     [&](const Jet* j){ return j->abseta() > 2.8 || deltaR_rap(*m, *j) > 0.4; }))
            signalMuons.push_back(m);

        // The subset of jets with pT > 50 GeV is used for several calculations
        vector<const Jet*> signalJets50;
        for (const Jet* j : signalJets)
          if (j->pT() > 50) signalJets50.push_back(j);


        ////////////////////////////////
        // Calculate common variables and cuts

        // Multiplicities
        const size_t nElectrons = signalElectrons.size();
        const size_t nMuons = signalMuons.size();
        const size_t nJets = signalJets.size();
        const size_t nJets50 = signalJets50.size();

        // HT-related quantities (calculated over all >20 GeV jets)
        double sumptj = 0;
        for (const Jet* j : signalJets) sumptj += j->pT();
        const double HT = sumptj;
        const double sqrtHT = sqrt(HT);
        const double met_sqrtHT = met/sqrtHT;

        // Meff-related quantities (calculated over >50 GeV jets only)
        double sumptj50_4 = 0, sumptj50_5 = 0, sumptj50_6 = 0, sumptj50_incl = 0;
        for (size_t i = 0; i < signalJets50.size(); ++i) {
          const Jet* j = signalJets50[i];
          if (i < 4) sumptj50_4 += j->pT();
          if (i < 5) sumptj50_5 += j->pT();
          if (i < 6) sumptj50_6 += j->pT();
          sumptj50_incl += j->pT();
        }
        const double meff_4 = met + sumptj50_4;
        const double meff_5 = met + sumptj50_5;
        const double meff_6 = met + sumptj50_6;
        const double meff_incl = met + sumptj50_incl;
        const double met_meff_4 = met / meff_4;
        const double met_meff_5 = met / meff_5;
        const double met_meff_6 = met / meff_6;

        // Jet |eta|s
        double etamax_2 = 0;
        for (size_t i = 0; i < min(2lu,signalJets50.size()); ++i)
          etamax_2 = max(etamax_2, signalJets[i]->abseta());
        double etamax_4 = etamax_2;
        for (size_t i = 2; i < min(4lu,signalJets50.size()); ++i)
          etamax_4 = max(etamax_4, signalJets[i]->abseta());
        double etamax_6 = etamax_4;
        for (size_t i = 4; i < min(6lu,signalJets50.size()); ++i)
          etamax_6 = max(etamax_6, signalJets[i]->abseta());

        // Jet--MET dphis
        double dphimin_123 = DBL_MAX, dphimin_more = DBL_MAX;
        for (size_t i = 0; i < min(3lu,signalJets50.size()); ++i)
          dphimin_123 = min(dphimin_123, acos(cos(signalJets50[i]->phi() - pmiss.phi())));
        for (size_t i = 3; i < signalJets50.size(); ++i)
          dphimin_more = min(dphimin_more, acos(cos(signalJets50[i]->phi() - pmiss.phi())));

        // Jet aplanarity
        /// @todo Computed over all jets, all >50 jets, or 4,5,6 jets? Currently using all (> 20) jets
        Eigen::Matrix3d momtensor = Eigen::Matrix3d::Zero();
        double norm = 0;
        for (const Jet* jet : signalJets) {
          const P4& p4 = jet->mom();
          norm += p4.p2();
          for (size_t i = 0; i < 3; ++i) {
            const double pi = (i == 0) ? p4.px() : (i == 1) ? p4.py() : p4.pz();
            for (size_t j = 0; j < 3; ++j) {
              const double pj = (j == 0) ? p4.px() : (j == 1) ? p4.py() : p4.pz();
              momtensor(i,j) += pi*pj;
            }
          }
        }
        momtensor *= norm;
        const double mineigenvalue = momtensor.eigenvalues().real().minCoeff();
        const double aplanarity = 1.5 * mineigenvalue;


        ////////////////////////////////
        // Fill signal regions


        const bool leptonCut = (nElectrons == 0 && nMuons == 0);
        const bool metCut = (met > 250.);
        if (nJets50 >= 2 && leptonCut && metCut) {

          // 2 jet regions
          if (dphimin_123 > 0.8 && dphimin_more > 0.4) {
            if (signalJets[1]->pT() > 200 && etamax_2 < 0.8) { //< implicit pT[0] cut
              if (met_sqrtHT > 14 && meff_incl >  800) _srnums[0] += 1;
            }
            if (signalJets[1]->pT() > 250 && etamax_2 < 1.2) { //< implicit pT[0] cut
              if (met_sqrtHT > 16 && meff_incl > 1200) _srnums[1] += 1;
              if (met_sqrtHT > 18 && meff_incl > 1600) _srnums[2] += 1;
              if (met_sqrtHT > 20 && meff_incl > 2000) _srnums[3] += 1;
            }
          }

          // 3 jet region
          if (nJets50 >= 3 && dphimin_123 > 0.4 && dphimin_more > 0.2) {
            if (signalJets[0]->pT() > 600 && signalJets[2]->pT() > 50) { //< implicit pT[1] cut
              if (met_sqrtHT > 16 && meff_incl > 1200) _srnums[4] += 1;
            }
          }

          // 4 jet regions (note implicit pT[1,2] cuts)
          if (nJets >= 4 && dphimin_123 > 0.4 && dphimin_more > 0.4 && signalJets[0]->pT() > 200 && aplanarity > 0.04) {
            if (signalJets[3]->pT() > 100 && etamax_4 < 1.2 && met_meff_4 > 0.25 && meff_incl > 1000) _srnums[5] += 1;
            if (signalJets[3]->pT() > 100 && etamax_4 < 2.0 && met_meff_4 > 0.25 && meff_incl > 1400) _srnums[6] += 1;
            if (signalJets[3]->pT() > 100 && etamax_4 < 2.0 && met_meff_4 > 0.20 && meff_incl > 1800) _srnums[7] += 1;
            if (signalJets[3]->pT() > 150 && etamax_4 < 2.0 && met_meff_4 > 0.20 && meff_incl > 2200) _srnums[8] += 1;
            if (signalJets[3]->pT() > 150 &&                   met_meff_4 > 0.20 && meff_incl > 2600) _srnums[9] += 1;
          }

          // 5 jet region (note implicit pT[1,2,3] cuts)
          if (nJets >= 5 && dphimin_123 > 0.4 && dphimin_more > 0.2 && signalJets[0]->pT() > 500) {
            if (signalJets[4]->pT() > 50 && met_meff_5 > 0.3 && meff_incl > 1400) _srnums[10] += 1;
          }

          // 6 jet regions (note implicit pT[1,2,3,4] cuts)
          if (nJets >= 6 && dphimin_123 > 0.4 && dphimin_more > 0.2 && signalJets[0]->pT() > 200 && aplanarity > 0.08) {
            if (signalJets[5]->pT() >  50 && etamax_6 < 2.0 && met_meff_6 > 0.20 && meff_incl > 1800) _srnums[11] += 1;
            if (signalJets[5]->pT() > 100 &&                   met_meff_6 > 0.15 && meff_incl > 2200) _srnums[12] += 1;
          }

        }

      }


      void add(BaseAnalysis* other) {
        // The base class add function handles the signal region vector and total # events.
        HEPUtilsAnalysis::add(other);

        Analysis_ATLAS_13TeV_0LEP_13invfb* specificOther = dynamic_cast<Analysis_ATLAS_13TeV_0LEP_13invfb*>(other);

        for (size_t i = 0; i < NUMSR; ++i)
          _srnums[i] += specificOther->_srnums[i];
      }


      /// Register results objects with the results for each SR; obs & bkg numbers from the CONF note
      void collect_results() {
        static const string ANAME = "Analysis_ATLAS_13TeV_0LEP_13invfb";
        add_result(SignalRegionData(ANAME, "meff-2j-0800", 650, {_srnums[0],  0.}, {610., 50.}));
        add_result(SignalRegionData(ANAME, "meff-2j-1200", 270, {_srnums[1],  0.}, {297., 29.}));
        add_result(SignalRegionData(ANAME, "meff-2j-1600",  96, {_srnums[2],  0.}, {121., 13.}));
        add_result(SignalRegionData(ANAME, "meff-2j-2000",  29, {_srnums[3],  0.}, { 42.,  6.}));
        add_result(SignalRegionData(ANAME, "meff-3j-1200", 363, {_srnums[4],  0.}, {355., 33.}));
        add_result(SignalRegionData(ANAME, "meff-4j-1000",  97, {_srnums[5],  0.}, { 84.,  7.}));
        add_result(SignalRegionData(ANAME, "meff-4j-1400",  71, {_srnums[6],  0.}, { 66.,  8.}));
        add_result(SignalRegionData(ANAME, "meff-4j-1800",  37, {_srnums[7],  0.}, { 27.,  3.2}));
        add_result(SignalRegionData(ANAME, "meff-4j-2200",  10, {_srnums[8],  0.}, {  4.8, 1.1}));
        add_result(SignalRegionData(ANAME, "meff-4j-2600",   3, {_srnums[9],  0.}, {  2.7, 0.6}));
        add_result(SignalRegionData(ANAME, "meff-5j-1400",  64, {_srnums[10], 0.}, { 68.,  9.}));
        add_result(SignalRegionData(ANAME, "meff-6j-1800",  10, {_srnums[11], 0.}, {  5.5, 1.0}));
        add_result(SignalRegionData(ANAME, "meff-6j-2200",   1, {_srnums[12], 0.}, {  0.82,0.35}));
      }

    };


    // Factory fn
    DEFINE_ANALYSIS_FACTORY(ATLAS_13TeV_0LEP_13invfb)


  }
}
