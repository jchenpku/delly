/*
============================================================================
DELLY: Structural variant discovery by integrated PE mapping and SR analysis
============================================================================
Copyright (C) 2012 Tobias Rausch

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
============================================================================
Contact: Tobias Rausch (rausch@embl.de)
============================================================================
*/

#ifndef BOLOG_H
#define BOLOG_H

#include <boost/math/special_functions/round.hpp>

namespace torali {

#define SMALLEST_GL -1000

template<typename TPrecision>
struct BoLog {
  std::vector<TPrecision> phred2prob;

  BoLog() {
    for(int i = 0; i <= boost::math::round(-10 * SMALLEST_GL); ++i) phred2prob.push_back(std::pow(TPrecision(10), -(TPrecision(i)/TPrecision(10))));
  }
};


 template<typename TMapqVector, typename FLP, typename TGQ>
 inline bool
 _computeGLs(TMapqVector const& mapqRef, TMapqVector const& mapqAlt, FLP* gl, TGQ& gq, int32_t* gts, int const file_c) {
   // Compute genotype likelihoods
   BoLog<FLP> bl;
   
   for(unsigned int geno=0; geno<=2; ++geno) gl[geno]=0;
   unsigned int peDepth=mapqRef.size() + mapqAlt.size();
   for(typename TMapqVector::const_iterator mapqRefIt = mapqRef.begin();mapqRefIt!=mapqRef.end();++mapqRefIt) {
     gl[0] += std::log10(bl.phred2prob[*mapqRefIt]);
     gl[1] += std::log10(bl.phred2prob[*mapqRefIt] + (FLP(1) - bl.phred2prob[*mapqRefIt]));
     gl[2] += std::log10(FLP(1) - bl.phred2prob[*mapqRefIt]);
   }
   for(typename TMapqVector::const_iterator mapqAltIt = mapqAlt.begin();mapqAltIt!=mapqAlt.end();++mapqAltIt) {
     gl[0] += std::log10(FLP(1) - bl.phred2prob[*mapqAltIt]);
     gl[1] += std::log10((FLP(1) - bl.phred2prob[*mapqAltIt]) + bl.phred2prob[*mapqAltIt]);
     gl[2] += std::log10(bl.phred2prob[*mapqAltIt]);
   }
   gl[1] += -FLP(peDepth) * std::log10(FLP(2));
   unsigned int glBest=0;
   FLP glBestVal=gl[glBest];
   for(unsigned int geno=1; geno<=2; ++geno) {
     if (gl[geno] > glBestVal) {
       glBestVal=gl[geno];
       glBest = geno;
     }
   }
   // Rescale by best genotype
   for(unsigned int geno=0; geno<=2; ++geno) {
     gl[geno] -= glBestVal;
     // Cap at smallest GL
     gl[geno] = (gl[geno] > SMALLEST_GL) ? gl[geno] : SMALLEST_GL;
   }

   // Phred-scaled genotype likelihoods
   uint32_t pl[3];
   pl[0] = (uint32_t) boost::math::round(-10 * gl[0]);
   pl[1] = (uint32_t) boost::math::round(-10 * gl[1]);
   pl[2] = (uint32_t) boost::math::round(-10 * gl[2]);
   if ((peDepth) && (pl[0] + pl[1] + pl[2] > 0)) {
     FLP likelihood = (FLP) std::log10((1-1/(bl.phred2prob[pl[0]]+bl.phred2prob[pl[1]]+bl.phred2prob[pl[2]])));
     likelihood = (likelihood > SMALLEST_GL) ? likelihood : SMALLEST_GL;
     gq = (TGQ) boost::math::round(-10 * likelihood);
     if (glBest==0) {
       gts[file_c * 2] = bcf_gt_unphased(1);
       gts[file_c * 2 + 1] = bcf_gt_unphased(1);
     } else if (glBest==1) {
       gts[file_c * 2] = bcf_gt_unphased(0);
       gts[file_c * 2 + 1] = bcf_gt_unphased(1);
     } else {
       gts[file_c * 2] = bcf_gt_unphased(0);
       gts[file_c * 2 + 1] = bcf_gt_unphased(0);
     }
   } else {
     gts[file_c * 2] = bcf_gt_missing;
     gts[file_c * 2 + 1] = bcf_gt_missing;
     gq = 0;
   }
   return (gq > 0);
 }
 

}

#endif
