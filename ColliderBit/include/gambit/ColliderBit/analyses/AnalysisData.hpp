#pragma once
//   GAMBIT: Global and Modular BSM Inference Tool
//   *********************************************
///  \file
///
///  The SignalRegionData and AnalysisData structs.

#include "gambit/ColliderBit/ColliderBit_macros.hpp"
#include "gambit/ColliderBit/Utils.hpp"

#include "HEPUtils/MathUtils.h"
#include "HEPUtils/Event.h"

#include "Eigen/Core"

#include <string>
#include <sstream>
#include <vector>
#include <cmath>
#include <cfloat>
#include <limits>
#include <memory>
#include <iomanip>
#include <algorithm>

namespace Gambit {
  namespace ColliderBit {


    /// A simple container for the result of one signal region from one analysis.
    struct SignalRegionData {

      /// Constructor with {n,nsys} pair args
      SignalRegionData(const std::string& name, const std::string& sr,
                       double nobs, const std::pair<double,double>& nsig, const std::pair<double,double>& nbkg,
                       double nsigatlumi=-1)
        : SignalRegionData(name, sr, nobs, nsig.first, nbkg.first,
                           nsig.second, nbkg.second, nsigatlumi)
      {    }

      /// Constructor with separate n & nsys args
      SignalRegionData(const std::string& name, const std::string& sr,
                       double nobs, double nsig, double nbkg,
                       double syssig, double sysbkg, double nsigatlumi=-1)
        : analysis_name(name), sr_label(sr),
          n_observed(nobs), n_signal(nsig), n_signal_at_lumi(nsigatlumi), n_background(nbkg),
          signal_sys(syssig), background_sys(sysbkg)
      {    }

      /// Default constructor
      SignalRegionData() {}

      /// @name Analysis and signal region specification
      //@{
      std::string analysis_name; ///< The name of the analysis common to all signal regions
      std::string sr_label; ///< A label for the particular signal region of the analysis
      //@}

      /// @name Signal region data
      //@{
      double n_observed = 0; ///< The number of events passing selection for this signal region as reported by the experiment
      double n_signal = 0; ///< The number of simulated model events passing selection for this signal region
      double n_signal_at_lumi = -1; ///< n_signal, scaled to the experimental luminosity
      double n_background = 0; ///< The number of standard model events expected to pass the selection for this signal region, as reported by the experiment.
      double signal_sys = 0; ///< The absolute systematic error of n_signal
      double background_sys = 0; ///< The absolute systematic error of n_background
      //@}

    };


    /// A container for the result of an analysis, potentially with many signal regions and correlations
    ///
    /// @todo Access by name?
    /// @todo Guarantee ordering?
    struct AnalysisData {

      /// Default constructor
      AnalysisData() { clear(); }

      /// @brief Constructor from a list of SignalRegionData and an optional correlation (or covariance?) matrix
      ///
      /// If corrs is a null matrix (the default), this AnalysisData is to be interpreted as having no correlation
      /// information, and hence the likelihood calculation should use the single best-expected-limit SR.
      AnalysisData(const std::vector<SignalRegionData>& srds, const Eigen::MatrixXd& corrs=Eigen::MatrixXd())
        : srdata(srds), corrmatrix(corrs) {
        _checkConsistency();
      }

      /// Clear the list of SignalRegionData, and nullify the correlation matrix
      void clear() {
        srdata.clear();
        corrmatrix = Eigen::MatrixXd();
      }

      /// Number of analyses
      size_t size() const {
        _checkConsistency();
        return srdata.size();
      }

      /// Is this container empty of signal regions?
      bool empty() const { return size() == 0; }

      /// Is there non-null correlation data?
      bool hasCorrs() const {
        _checkConsistency();
        return corrmatrix.rows() == 0;
      }

      /// @brief Add a SignalRegionData
      /// @todo Allow naming the SRs?
      void add(const SignalRegionData& srd) {
        srdata.push_back(srd);
      }

      /// Access the i'th signal region's data
      SignalRegionData& operator[] (size_t i) { return srdata[i]; }
      /// Access the i'th signal region's data (const)
      const SignalRegionData& operator[] (size_t i) const { return srdata[i]; }

      /// Iterators (sugar for direct access to this->srdata)
      std::vector<SignalRegionData>::iterator begin() { return srdata.begin(); }
      std::vector<SignalRegionData>::const_iterator begin() const { return srdata.begin(); }
      std::vector<SignalRegionData>::iterator end() { return srdata.end(); }
      std::vector<SignalRegionData>::const_iterator end() const { return srdata.end(); }

      /// List of signal regions' data sumamries
      std::vector<SignalRegionData> srdata;

      /// Optional matrix of correlations between SRs (0x0 null matrix = no corr info)
      Eigen::MatrixXd corrmatrix;

      /// Check that the size of the SRData list and the correlation matrix are consistent
      void _checkConsistency() const {
        assert(corrmatrix.rows() == 0 || corrmatrix.rows() == (int) srdata.size());
      }

    };


  }
}
