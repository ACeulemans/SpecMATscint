///Author: Oleksii Poleshchuk
///
///KU Leuven 2016-2019
///
///SpecMATscint is a GEANT4 code for simulation
///of gamma-rays detection efficiency with
///the SpecMAT scintillation array.
///
///Primarily, this code was written for identification of
///the best geometry of a scintillation array based
///on it's total detection efficiency.
///
/// \file SpecMATSimEventAction.cc
/// \brief Implementation of the SpecMATSimEventAction class

#include "SpecMATSimEventAction.hh"
#include "SpecMATSimRunAction.hh"
#include "SpecMATSimAnalysis.hh"
#include "SpecMATSimDetectorConstruction.hh"

#include "G4RunManager.hh"
#include "G4Event.hh"
#include "G4SDManager.hh"
#include "G4HCofThisEvent.hh"
#include "G4GenericMessenger.hh"
#include "G4THitsMap.hh"
#include "G4THitsCollection.hh"
#include "G4UnitsTable.hh"
#include "G4SystemOfUnits.hh"
#include "G4THitsCollection.hh"

#include "Randomize.hh"
#include <iomanip>
#include <cmath>

// ###################################################################################

SpecMATSimEventAction::SpecMATSimEventAction(SpecMATSimRunAction* runAction)
: G4UserEventAction(),
  sciCryst(0),
  fRunAct(runAction),
  fCollID_cryst(0.),
  fCollID_ComptSupp(0.),
  fPrintModulo(1)
{
  sciCryst = new SpecMATSimDetectorConstruction();
}

// ###################################################################################

SpecMATSimEventAction::~SpecMATSimEventAction()
{

}

// ###################################################################################

G4THitsMap<G4double>*SpecMATSimEventAction::GetHitsCollection(const G4String& hcName, const G4Event* event) const
{
  G4int hcID = G4SDManager::GetSDMpointer()->GetCollectionID(hcName);
  G4THitsMap<G4double>* hitsCollection = static_cast<G4THitsMap<G4double>*>(event->GetHCofThisEvent()->GetHC(hcID));
  if ( ! hitsCollection ) {
    G4cerr << "Cannot access hitsCollection " << hcName << G4endl;
    exit(1);
  }
  return hitsCollection;
}

G4double SpecMATSimEventAction::GetSum(G4THitsMap<G4double>* hitsMap) const
{
  G4double sumValue = 0;
  std::map<G4int, G4double*>::iterator it;
  for ( it = hitsMap->GetMap()->begin(); it != hitsMap->GetMap()->end(); it++) {
    sumValue += *(it->second);
  }
  return sumValue;
}
// ###################################################################################

void SpecMATSimEventAction::BeginOfEventAction(const G4Event* event )
{
  eventNb = event->GetEventID();
  G4cout << "\n###########################################################" << G4endl;
  G4cout << "Event №" << eventNb << G4endl;
  ComptSuppFlagTest = sciCryst->GetComptSuppFlag();
  if (eventNb == 0) {
    SDMan = G4SDManager::GetSDMpointer();
    fCollID_cryst   = SDMan->GetCollectionID("crystal/edep");
    if (ComptSuppFlagTest == "yes") {
      fCollID_ComptSupp = +SDMan->GetCollectionID("ComptSupp/edep");
    }
  }
  /*
  if (eventNb%fPrintModulo == 0) {
  G4cout << "\n---> Begin of event: " << eventNb << G4endl;
}
*/
}

// ###################################################################################

void SpecMATSimEventAction::EndOfEventAction(const G4Event* event )
{
  analysisManager = G4AnalysisManager::Instance();
  eventNb = event->GetEventID();
  analysisManager->FillNtupleDColumn(0, eventNb);
  //Hits collections
  //
  HCE = event->GetHCofThisEvent();
  if(!HCE) return;

  //Energy in crystals : identify 'good events'
  //
  const G4double eThreshold = 0*eV;
  nbOfFired = 0;

  G4THitsMap<G4double>* eventMapCryst = (G4THitsMap<G4double>*)(HCE->GetHC(fCollID_cryst));

  std::map<G4int,G4double*>::iterator itr;

  for (itr = eventMapCryst->GetMap()->begin(); itr != eventMapCryst->GetMap()->end(); itr++) {
    copyNb  = (itr->first);
    edep = *(itr->second);
    if (edep > eThreshold) nbOfFired++;
    crystMat = sciCryst->GetSciCrystMat();

    if (crystMat->GetName() == "CeBr3") {
      //Resolution correction of registered gamma energy for CeBr3.
      //absoEdep = G4RandGauss::shoot(edep/keV, (((edep/keV)*(108*pow(edep/keV, -0.498))/100)/2.355)); //Quarati [NIM A 729 (2013) 596–604]
      absoEdep = G4RandGauss::shoot(edep/keV, (((edep/keV)*(94.6*pow(edep/keV, -0.476))/100)/2.355)); //KUL 10 measurement with GET
    }
    else if (crystMat->GetName() == "LaBr3") {
      //Resolution correction of registered gamma energy for LaBr3.
      absoEdep = G4RandGauss::shoot(edep/keV, (((edep/keV)*(81*pow(edep/keV, -0.501))/100)/2.355)); //Quarati [NIM A 729 (2013) 596–604]
    }
    else {
      absoEdep = edep/keV;
    }

    //G4cout << "\n" << crystMat->GetName() +  " Nb" << copyNb << ": E " << edep/keV << " keV, Resolution Corrected E "<< absoEdep << " keV, " << "FWHM " << ((edep/keV)*(108*pow(edep/keV,-0.498))/100) << G4endl;

    // get analysis manager
    //

    // fill histograms
    //
    if (copyNb <= ((sciCryst->GetNbCrystInSegmentRow())*(sciCryst->GetNbCrystInSegmentColumn())*(sciCryst->GetNbSegments())+1)) {
      analysisManager->FillH1(copyNb, absoEdep); //each crystal EdepRes
      analysisManager->FillH1((sciCryst->GetNbCrystInSegmentRow())*(sciCryst->GetNbCrystInSegmentColumn())*(sciCryst->GetNbSegments())+1, absoEdep); //total EdepRes
      analysisManager->FillH1((sciCryst->GetNbCrystInSegmentRow())*(sciCryst->GetNbCrystInSegmentColumn())*(sciCryst->GetNbSegments())+4, edep/keV); //total EdepNoRes
      //analysisManager->FillH1(copyNb, edep/keV);
      analysisManager->FillNtupleDColumn(0, eventNb);
      analysisManager->FillNtupleDColumn(1, copyNb);
      analysisManager->FillNtupleDColumn(2, absoEdep);
      analysisManager->FillNtupleDColumn(7, edep/keV);
      if (copyNb == 1) {
        analysisManager->FillNtupleDColumn(3, absoEdep);
        analysisManager->FillNtupleDColumn(8, edep/keV);
      }
      if (copyNb == 2) {
        analysisManager->FillNtupleDColumn(4, absoEdep);
        analysisManager->FillNtupleDColumn(9, edep/keV);
      }
      if (copyNb != 3 && copyNb != 6 && copyNb != 9 && copyNb != 12 && copyNb != 15 && copyNb != 18 && copyNb != 21 && copyNb != 24 && copyNb != 27 && copyNb != 30 && copyNb != 33 && copyNb != 36 && copyNb != 39 && copyNb != 42 && copyNb != 45) {
        //30 crystals (without the ring located further from the beamline)
        analysisManager->FillNtupleDColumn(5, absoEdep);
        analysisManager->FillNtupleDColumn(10, edep/keV);//30 crystals (without the ring located further from the beamline)
        analysisManager->FillH1((sciCryst->GetNbCrystInSegmentRow())*(sciCryst->GetNbCrystInSegmentColumn())*(sciCryst->GetNbSegments())+2, absoEdep); //total EdepRes for 30Cryst
        analysisManager->FillH1((sciCryst->GetNbCrystInSegmentRow())*(sciCryst->GetNbCrystInSegmentColumn())*(sciCryst->GetNbSegments())+5, edep/keV); //total EdepNoRes for 40Cryst
      }
      if (copyNb != 3 && copyNb != 6 && copyNb != 9 && copyNb != 12 && copyNb != 15) {
        //40 crystals (without five in the ring located further from the beamline)
        analysisManager->FillNtupleDColumn(6, absoEdep);
        analysisManager->FillNtupleDColumn(11, edep/keV);
        analysisManager->FillH1((sciCryst->GetNbCrystInSegmentRow())*(sciCryst->GetNbCrystInSegmentColumn())*(sciCryst->GetNbSegments())+3, absoEdep); //total EdepRes for 40Cryst
        analysisManager->FillH1((sciCryst->GetNbCrystInSegmentRow())*(sciCryst->GetNbCrystInSegmentColumn())*(sciCryst->GetNbSegments())+6, edep/keV); //total EdepNoRes for 40Cryst
      }
      analysisManager->AddNtupleRow();
    }
  }

  ComptSuppFlagTest = sciCryst->GetComptSuppFlag();
  if (ComptSuppFlagTest == "yes") {

    nbOfFiredComptSupp = 0;
    G4THitsMap<G4double>* eventMapComptSupp = (G4THitsMap<G4double>*)(HCE->GetHC(fCollID_ComptSupp));

    std::map<G4int,G4double*>::iterator itr2;

    for (itr2 = eventMapComptSupp->GetMap()->begin(); itr2 != eventMapComptSupp->GetMap()->end(); itr2++) {
      copyNbComptSupp  = (itr2->first);
      edepComptSupp = *(itr2->second);
      if (edepComptSupp > eThreshold) nbOfFiredComptSupp++;
      //Resolution correction of registered gamma energy for BGO.
      edepComptSuppRes = G4RandGauss::shoot(edepComptSupp/keV, (((edepComptSupp/keV)*(398*pow(edepComptSupp/keV, -0.584))/100)/2.355));

      //G4cout << "\n" << "ComptSupp Nb" << copyNbComptSupp << ": E " << edepComptSupp/keV << " keV, Resolution Corrected E "<< edepComptSuppRes << " keV, " << "FWHM " << ((edep/keV)*(108*pow(edep/keV,-0.498))/100) << G4endl;

      // fill histograms
      //
      if (copyNbComptSupp > (99)) {
        analysisManager->FillH1((sciCryst->GetNbCrystInSegmentRow())*(sciCryst->GetNbCrystInSegmentColumn())*(sciCryst->GetNbSegments())+2+copyNbComptSupp-100, edepComptSuppRes);
        analysisManager->FillNtupleDColumn(0, eventNb);
        analysisManager->FillNtupleDColumn(12, eventNb);
        analysisManager->FillNtupleDColumn(13, copyNbComptSupp);
        analysisManager->FillNtupleDColumn(14, edepComptSuppRes);
        analysisManager->FillNtupleDColumn(15, edepComptSupp/keV);
        analysisManager->AddNtupleRow();

      }
    }
  }
}

// ###################################################################################
