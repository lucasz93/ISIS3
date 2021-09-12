/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Preference.h"

#include <iostream>
#include <iomanip>
#include <algorithm>

#include "Kernels.h"

//  Used extensively in Kernels interface
using namespace std;
using namespace Isis;

/**
 *  
 * @internal 
 *   @history 2015-02-25 Jeannie Backer - Added hayabusa test for DSK support. Unable to test
 *                           resolveTypeByExt() for DSK files. NAIF is expected to be able to
 *                           determine type from the file itself.
 *                           Current Code coverage: 67% scope, 81% line, and 80% function.
 *
 */

QString stripPath(QString input) {
  QString result = input.replace(
      QRegExp("(.*/)([^/]*/[^/]*/[^/]*/[^/]*$)"),
      "$\\2");

  return result;
}

int main(int argc, char *argv[]) {
  Isis::Preference::Preferences(true);
  NaifContextLifecycle naif_lifecycle;
  auto naif = NaifContext::acquire();
  QString inputFile = "$ISISTESTDATA/isis/src/mgs/unitTestData/ab102401.lev2.cub";
  if (--argc == 1) { inputFile = argv[1]; }

  cout << "\n\nTesting Kernels class using file " << inputFile << "\n";

  Kernels myKernels(inputFile);
  cout << "\nList of kernels found - Total: " << myKernels.size() << "\n";
  QStringList kfiles = myKernels.getKernelList();
  transform(kfiles.begin(), kfiles.end(), kfiles.begin(), &stripPath);
  cout << kfiles.join("\n") << endl;

  cout << "\nTypes of kernels found\n";
  QStringList ktypes = myKernels.getKernelTypes();
  cout << ktypes.join("\n") << endl;

  //  Test to see if we have any kernels loaded at all
  Kernels query;
  query.Discover(naif);
  cout << "\nInitial currently loaded kernel files = " << query.size() << "\n";
  QStringList kloaded = query.getKernelList();
  transform(kloaded.begin(), kloaded.end(), kloaded.begin(), &stripPath);
  cout << kloaded.join("\n") << endl;

  //  Load all the kernels
  myKernels.Load(naif);
  query.Discover(naif);
  cout << "\nAfter LoadALL option, kernels loaded = " << query.size() << "\n";
  kloaded = query.getKernelList();
  transform(kloaded.begin(), kloaded.end(), kloaded.begin(), &stripPath);
  cout << kloaded.join("\n") << endl;

  // Unload and check for proper status
  myKernels.UnLoad(naif);
  query.Discover(naif);
  cout << "\nUnLoading All, count after = " << query.size() << "\n";

  //  Now load the SPK kernels after unloading
  myKernels.Load(naif, "SPK");
  query.Discover(naif);
  cout << "\nLoaded SPK kernels = " << query.size() << "\n";
  kloaded = query.getKernelList();
  transform(kloaded.begin(), kloaded.end(), kloaded.begin(), &stripPath);
  cout << kloaded.join("\n") << endl;

  // Load kernels needed for Time manipulation
  myKernels.Load(naif, "LSK,SCLK");
  myKernels.UnLoad(naif, "SPK");
  query.Discover(naif);
  cout << "\nLoad LSK, SCLK for Time manip, unload SPK kernels = " << query.size() << "\n";
  kloaded = query.getKernelList();
  transform(kloaded.begin(), kloaded.end(), kloaded.begin(), &stripPath);
  cout << kloaded.join("\n") << endl;

  // Check double load behavior
  Kernels clone;
  clone.Merge(query);
  clone.Manage();
  clone.UnLoad(naif);

  myKernels.UpdateLoadStatus(naif);
  cout << "\nNumber loaded: " << myKernels.getLoadedList().size() << "\n";
  myKernels.Load(naif,"LSK,SCLK");
  // Load same files
  clone.Load(naif);
  query.Discover(naif);
  cout << "\nCheck Double-Load of LSK, SCLK = " << query.size() << "\n";
  kloaded = query.getKernelList();
  transform(kloaded.begin(), kloaded.end(), kloaded.begin(), &stripPath);
  cout << kloaded.join("\n") << endl;

  // Unload each set
  clone.UnLoad(naif);
  query.Discover(naif);
  cout << "\nUnload the cloned set = " << query.size() << "\n";
  kloaded = query.getKernelList();
  transform(kloaded.begin(), kloaded.end(), kloaded.begin(), &stripPath);
  cout << kloaded.join("\n") << endl;
  clone.UnManage();

  //  Load SPK set
  myKernels.UnLoad(naif);
  myKernels.Load(naif, "LSK,FK,DAF,SPK");
  query.Discover(naif);
  cout << "\nCheck SPK load  (LSK,FK,DAF,SPK)= " << query.size() << "\n";
  kloaded = query.getKernelList();
  transform(kloaded.begin(), kloaded.end(), kloaded.begin(), &stripPath);
  cout << kloaded.join("\n") << endl;

  // Now unload SPKs, preserve LSK and load CK stuff
  myKernels.UnLoad(naif, "DAF,SPK");
  cout << "Unload DAF,SPK\n";
  myKernels.Load(naif, "SCLK,IK,CK");
  query.Discover(naif);
  cout << "\nCheck CK load  (SCLK,IK,CK) = " << query.size() << "\n";
  kloaded = query.getKernelList();
  transform(kloaded.begin(), kloaded.end(), kloaded.begin(), &stripPath);
  cout << kloaded.join("\n") << endl;

  // Now reload all and check
  myKernels.Load(naif, "LSK,FK,SCLK,IK,CK");
  query.Discover(naif);
  cout << "\nCheck CK reload  (LSK,FK,SCLK,IK,CK) = " << query.size() << "\n";
  kloaded = query.getKernelList();
  transform(kloaded.begin(), kloaded.end(), kloaded.begin(), &stripPath);
  cout << kloaded.join("\n") << endl;

  //  Clear the pool and start fresh.  Clear all instances and reinitialize NAIF
  clone.Clear();
  query.Clear();
  myKernels.Clear();
  myKernels.InitializeNaifKernelPool(naif);

  // Left two kernels open, ensure we have none left
  query.Discover(naif);
  cout << "\n\nEnsure clean pool...Count: " << query.size() << "\n";

  // Load a bogus file and check for missing
  myKernels.Add(naif, "$base/kernels/lsk/dne.lsk");
  cout << "\nLoad of bogus file, should have one missing: " << myKernels.Missing()
       << "\n";
  myKernels.Clear();

  // Now add a set be hand
  myKernels.Add(naif, "$base/kernels/lsk/naif0009.tls");
  myKernels.Add(naif, "$base/kernels/spk/de405.bsp");
  myKernels.Add(naif, "$clementine1/kernels/ck/clem_ulcn2005_type2_1sc.bc");
  myKernels.Add(naif, "$clementine1/kernels/fk/clem_v11.tf");
  myKernels.Add(naif, "$clementine1/kernels/sclk/dspse002.tsc");
  myKernels.Add(naif, "$clementine1/kernels/spk/SPKMERGE_940219_940504_CLEMV001b.bsp");
  myKernels.Add(naif, "$clementine1/kernels/iak/uvvisAddendum003.ti");

  cout << "\n\nAdd Kernels directly - Count: " << myKernels.size() 
       << ", Missing: "<< myKernels.Missing() << "\n";
  cout << "\nList of kernels in object..\n";
  kfiles = myKernels.getKernelList();
  transform(kfiles.begin(), kfiles.end(), kfiles.begin(), &stripPath);
  cout << kfiles.join("\n") << endl;

  cout << "\nList of kernel types\n";
  ktypes = myKernels.getKernelTypes();
  cout << ktypes.join("\n") << endl;

  // Find unknown types
  kfiles = myKernels.getKernelList("UNKNOWN");
  cout << "\nUnknown kernels in list: " << kfiles.size() << "\n";
  cout << kfiles.join("\n") << endl;


  // Load them all
  myKernels.Load(naif);
  kloaded = myKernels.getLoadedList();
  transform(kloaded.begin(), kloaded.end(), kloaded.begin(), &stripPath);
  cout << "\nLoading all, total loaded: " << kloaded.size() << "\n";
  cout << kloaded.join("\n") << endl;

  //  Now double check list
  query.Discover(naif);
  cout << "\nCheck Load Status = " << query.size() << "\n";
  kloaded = query.getKernelList();
  transform(kloaded.begin(), kloaded.end(), kloaded.begin(), &stripPath);
  cout << kloaded.join("\n") << endl;

  // Unload SPK and CKs
  myKernels.UnLoad(naif, "SPK,CK");
  query.Discover(naif);
  cout << "\nUnload SPK,CK - Loaded: " << query.size() << "\n";
  kloaded = query.getKernelList();
  transform(kloaded.begin(), kloaded.end(), kloaded.begin(), &stripPath);
  cout << kloaded.join("\n") << endl;

  //  Clear the pool and start fresh.  Clear all instances and reinitialize NAIF
  clone.Clear();
  query.Clear();
  myKernels.Clear();
  myKernels.InitializeNaifKernelPool(naif);

  // Left two kernels open, ensure we have none left
  query.Discover(naif);
  cout << "\n\nEnsure clean pool...Count: " << query.size() << "\n";


  // Now add a set be hand
  myKernels.Add(naif, "$base/kernels/lsk/naif0009.tls");
  myKernels.Add(naif, "$base/kernels/pck/pck00009.tpc");
  myKernels.Add(naif, "$hayabusa/kernels/pck/itokawa_gaskell_n3.tpc");
  myKernels.Add(naif, "$hayabusa/kernels/tspk/de403s.bsp");
  myKernels.Add(naif, "$hayabusa/kernels/tspk/sb_25143_140.bsp");
  myKernels.Add(naif, "$hayabusa/kernels/spk/hay_jaxa_050916_051119_v1n.bsp");
  myKernels.Add(naif, "$hayabusa/kernels/spk/hay_osbj_050911_051118_v1n.bsp");
  myKernels.Add(naif, "$hayabusa/kernels/ck/hayabusa_itokawarendezvous_v02n.bc");
  myKernels.Add(naif, "$hayabusa/kernels/fk/hayabusa_hp.tf");
  myKernels.Add(naif, "$hayabusa/kernels/fk/itokawa_fixed.tf");
  myKernels.Add(naif, "$hayabusa/kernels/ik/amica31.ti");
  myKernels.Add(naif, "$hayabusa/kernels/iak/amicaAddendum001.ti");
  myKernels.Add(naif, "$hayabusa/kernels/sclk/hayabusa.tsc");
  myKernels.Add(naif, "$hayabusa/kernels/dsk/hay_a_amica_5_itokawashape_v1_0_512q.bds");

  cout << "\n\nAdd DSK Kernels directly - Count: " << myKernels.size() 
       << ", Missing: "<< myKernels.Missing() << "\n";
  cout << "\nList of kernels in object..\n";
  kfiles = myKernels.getKernelList();
  transform(kfiles.begin(), kfiles.end(), kfiles.begin(), &stripPath);
  cout << kfiles.join("\n") << endl;

  cout << "\nList of kernel types\n";
  ktypes = myKernels.getKernelTypes();
  cout << ktypes.join("\n") << endl;

  // Find unknown types
  kfiles = myKernels.getKernelList("UNKNOWN");
  cout << "\nUnknown kernels in list: " << kfiles.size() << "\n";
  cout << kfiles.join("\n") << endl;


  // Load them all
  myKernels.Load(naif);
  kloaded = myKernels.getLoadedList();
  transform(kloaded.begin(), kloaded.end(), kloaded.begin(), &stripPath);
  cout << "\nLoading all, total loaded: " << kloaded.size() << "\n";
  cout << kloaded.join("\n") << endl;

  //  Now double check list
  query.Discover(naif);
  cout << "\nCheck Load Status = " << query.size() << "\n";
  kloaded = query.getKernelList();
  transform(kloaded.begin(), kloaded.end(), kloaded.begin(), &stripPath);
  cout << kloaded.join("\n") << endl;

  // Unload SPK and CKs
  myKernels.UnLoad(naif, "SPK,CK");
  query.Discover(naif);
  cout << "\nUnload SPK,CK - Loaded: " << query.size() << "\n";
  kloaded = query.getKernelList();
  transform(kloaded.begin(), kloaded.end(), kloaded.begin(), &stripPath);
  cout << kloaded.join("\n") << endl;

  myKernels.UnLoad(naif);
  query.Discover(naif);
  cout << "\n\nAll Done - Should be 0 discovered: " << query.size() << "\n";
  // All done...
  return (0);
}
