/**
 * @file
 * $Revision: 1.0 $
 * $Date: 2021/08/21 20:02:37 $
 *
 *   Unless noted otherwise, the portions of Isis written by the USGS are public
 *   domain. See individual third-party library and package descriptions for
 *   intellectual property information,user agreements, and related information.
 *
 *   Although Isis has been used by the USGS, no warranty, expressed or implied,
 *   is made by the USGS as to the accuracy and functioning of such software
 *   and related material nor shall the fact of distribution constitute any such
 *   warranty, and no responsibility is assumed by the USGS in connection
 *   therewith.
 *
 *   For additional information, launch
 *   $ISISROOT/doc//documents/Disclaimers/Disclaimers.html in a browser or see
 *   the Privacy &amp; Disclaimers page on the Isis website,
 *   http://isis.astrogeology.usgs.gov, and the USGS privacy and disclaimers on
 *   http://www.usgs.gov/privacy.html.
 */
#include "NaifContext.h"
#include "NaifStatus.h"

#include "IException.h"
#include "IString.h"
#include "Pvl.h"
#include "PvlToPvlTranslationManager.h"

#include "cspice_state.h"

extern "C" {
#include <SpiceZpr.h>
#include <SpiceZfc.h>
}

#include <boost/make_shared.hpp>

namespace Isis {

  thread_local NaifContext* tls_naif_context = nullptr;
  thread_local int          tls_refcount = 0;

  void NaifContext::incrementRefcount() {
    if (!tls_refcount)
      tls_naif_context = new NaifContext();
    tls_refcount++;
  }

  void NaifContext::decrementRefcount() {
    if (tls_refcount == 0)
      throw std::logic_error("NaifContext refcount already at zero!");

    tls_refcount--;
    if (!tls_refcount) {
      delete tls_naif_context;
      tls_naif_context = nullptr;
    }
  }

  NaifContext* NaifContext::acquire() {    
    return tls_naif_context;
  }

  void NaifContext::attach(boost::shared_ptr<Internal> internal) {
    if (tls_refcount)
      throw std::runtime_error("Thread already has a NaifContext. Detach it or remove all references.");

    tls_naif_context = internal->m_context;
    tls_refcount = internal->m_refcount;

    // Zero the imported data so it doesn't get deleted when going out of scope.
    internal->m_context = nullptr;
    internal->m_refcount = 0;
  }

  boost::shared_ptr<NaifContext::Internal> NaifContext::detach() {
    auto internal = boost::make_shared<Internal>();
    internal->m_context = tls_naif_context;
    internal->m_refcount = tls_refcount;

    tls_refcount = 0;
    tls_naif_context = nullptr;

    return internal;
  }

  NaifContext::NaifContext() : m_naif(cspice_alloc(), &cspice_free) {}

  /**
   * This method looks for any naif errors that might have occurred. It
   * then compares the error to a list of known naif errors and converts
   * the error into an iException.
   *
   * @param resetNaif True if the NAIF error status should be reset (naif calls valid)
   */
  void NaifContext::CheckErrors(bool resetNaif) {
    NaifStatus::CheckErrors(this, resetNaif);
  }

  int NaifContext:: bodeul_(integer *body, doublereal *et, doublereal *ra, doublereal *dec, doublereal *w, doublereal *lamda) {
    return ::bodeul_(m_naif.get(), body, et, ra, dec, w, lamda);
  }

  int NaifContext::ckfrot_(integer *inst, doublereal *et, doublereal *rotate, integer *ref, logical *found) {
    return ::ckfrot_(m_naif.get(), inst, et, rotate, ref, found);
  }

  int NaifContext::drotat_(doublereal *angle, integer *iaxis, doublereal *dmout) {
    return ::drotat_(m_naif.get(), angle, iaxis, dmout);
  }

  int NaifContext::frmchg_(integer *frame1, integer *frame2, doublereal *et, doublereal *rotate) {
    return ::frmchg_(m_naif.get(), frame1, frame2, et, rotate);
  }

  int NaifContext::getlms_(char *msg, ftnlen msg_len) {
    return ::getlms_(m_naif.get(), msg, msg_len);
  }

  int NaifContext::invstm_(doublereal *mat, doublereal *invmat) {
    return ::invstm_(m_naif.get(), mat, invmat);
  }

  int NaifContext::refchg_(integer *frame1, integer *frame2, doublereal *et, doublereal *rotate) {
    return ::refchg_(m_naif.get(), frame1, frame2, et, rotate);
  }

  int NaifContext::tkfram_(integer *id, doublereal *rot, integer *frame, logical *found) {
    return ::tkfram_(m_naif.get(), id, rot, frame, found);
  }

  int NaifContext::zzdynrot_(integer *infram,  integer *center, doublereal *et, doublereal *rotate, integer *basfrm) {
    return ::zzdynrot_(m_naif.get(), infram, center, et, rotate, basfrm);
  }

}
