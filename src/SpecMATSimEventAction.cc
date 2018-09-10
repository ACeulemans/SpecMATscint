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

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

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

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

SpecMATSimEventAction::~SpecMATSimEventAction()
{

}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4THitsMap<G4double>*
SpecMATSimEventAction::GetHitsCollection(const G4String& hcName,
                                         const G4Event* event) const
{
  G4int hcID
    = G4SDManager::GetSDMpointer()->GetCollectionID(hcName);
  G4THitsMap<G4double>* hitsCollection
    = static_cast<G4THitsMap<G4double>*>(
        event->GetHCofThisEvent()->GetHC(hcID));

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
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void SpecMATSimEventAction::BeginOfEventAction(const G4Event* event )
{
  G4int eventNb = event->GetEventID();
  G4cout << "\n###########################################################" << G4endl;
  G4cout << "Event №" << eventNb << G4endl;

  if (eventNb == 0) {
    G4SDManager* SDMan = G4SDManager::GetSDMpointer();
    fCollID_cryst   = SDMan->GetCollectionID("crystal/edep");
    fCollID_ComptSupp = +SDMan->GetCollectionID("ComptSupp/edep");
  }

  if (eventNb%fPrintModulo == 0) {
    G4cout << "\n---> Begin of event: " << eventNb << G4endl;
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void SpecMATSimEventAction::EndOfEventAction(const G4Event* event )
{
  G4int eventNb = event->GetEventID();
  //Hits collections
  //
  G4HCofThisEvent* HCE = event->GetHCofThisEvent();
  if(!HCE) return;

  //Energy in crystals : identify 'good events'
  //
  const G4double eThreshold = 0*eV;
  G4int nbOfFired = 0;

  G4THitsMap<G4double>* eventMapCryst =
                     (G4THitsMap<G4double>*)(HCE->GetHC(fCollID_cryst));

  std::map<G4int,G4double*>::iterator itr;

  for (itr = eventMapCryst->GetMap()->begin(); itr != eventMapCryst->GetMap()->end(); itr++) {
    copyNb  = (itr->first);
    edep = *(itr->second);
    if (edep > eThreshold) nbOfFired++;
    crystMat = sciCryst->GetSciCrystMat();

    if (crystMat->GetName() == "CeBr3") {
    //Resolution correction of registered gamma energy for CeBr3.
    absoEdep = G4RandGauss::shoot(edep/keV, (((edep/keV)*(108*pow(edep/keV, -0.498))/100)/2.355));
    }
    else if (crystMat->GetName() == "LaBr3") {
    //Resolution correction of registered gamma energy for LaBr3.
    absoEdep = G4RandGauss::shoot(edep/keV, (((edep/keV)*(81*pow(edep/keV, -0.501))/100)/2.355));
    }
    else {
    absoEdep = edep/keV;
    }

    G4cout << "\n" << crystMat->GetName() +  " Nb" << copyNb << ": E " << edep/keV << " keV, Resolution Corrected E "<< absoEdep << " keV, " << "FWHM " << ((edep/keV)*(108*pow(edep/keV,-0.498))/100) << G4endl;

    // get analysis manager
    //
    G4AnalysisManager* analysisManager = G4AnalysisManager::Instance();

    // fill histograms
    //
    if (copyNb <= ((sciCryst->GetNbCrystInSegmentRow())*(sciCryst->GetNbCrystInSegmentColumn())*(sciCryst->GetNbSegments())+1)) {
    analysisManager->FillH1((sciCryst->GetNbCrystInSegmentRow())*(sciCryst->GetNbCrystInSegmentColumn())*(sciCryst->GetNbSegments())+1, absoEdep);
    analysisManager->FillH1(copyNb, absoEdep);
    }







    G4int nbOfFiredComptSupp = 0;
    G4THitsMap<G4double>* eventMapComptSupp =
                       (G4THitsMap<G4double>*)(HCE->GetHC(fCollID_ComptSupp));

    std::map<G4int,G4double*>::iterator itr;

    for (itr = eventMapComptSupp->GetMap()->begin(); itr != eventMapComptSupp->GetMap()->end(); itr++) {
      copyNbComptSupp  = (itr->first);
      edepComptSupp = *(itr->second);
      if (edepComptSupp > eThreshold) nbOfFiredComptSupp++;

      edepComptSuppRes = G4RandGauss::shoot(edepComptSupp/keV, (((edepComptSupp/keV)*(108*pow(edepComptSupp/keV, -0.498))/100)/2.355));


      G4cout << "\n" << "ComptSupp Nb" << copyNbComptSupp << ": E " << edepComptSupp/keV << " keV, Resolution Corrected E "<< edepComptSuppRes << " keV, " << "FWHM " << ((edep/keV)*(108*pow(edep/keV,-0.498))/100) << G4endl;

      // get analysis manager
      //
      G4AnalysisManager* analysisManager = G4AnalysisManager::Instance();

      // fill histograms
      //

      if (copyNbComptSupp > (99)) {
          analysisManager->FillH1((sciCryst->GetNbCrystInSegmentRow())*(sciCryst->GetNbCrystInSegmentColumn())*(sciCryst->GetNbSegments())+1+copyNbComptSupp-100, edepComptSuppRes);
      }
    }
    // fill ntuple
    //
    analysisManager->FillNtupleDColumn(0, eventNb);
    analysisManager->FillNtupleDColumn(1, copyNb);
    analysisManager->FillNtupleDColumn(2, absoEdep);
    analysisManager->FillNtupleDColumn(3, edep/keV);
    analysisManager->FillNtupleDColumn(4, copyNbComptSupp);
    analysisManager->FillNtupleDColumn(5, edepComptSuppRes);
    analysisManager->FillNtupleDColumn(6, edepComptSupp/keV);
    analysisManager->AddNtupleRow();
  }
}
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
