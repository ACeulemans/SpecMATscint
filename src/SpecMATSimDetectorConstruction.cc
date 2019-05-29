///Author: Oleksii Poleshchuk
///
///KU Leuven 2019
///
///SpecMATscint is a GEANT4 code for simulation
///of gamma-rays detection efficiency with
///the SpecMAT scintillation array.
///
///Primarily, this code was written for identification of
///the best geometry of a scintillation array based
///on it's total detection efficiency.
///
/// \file SpecMATSimDetectorConstruction.cc
/// \brief Implementation of the SpecMATSimDetectorConstruction class

#include "SpecMATSimDetectorConstruction.hh"

#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Trap.hh"
#include "G4Tubs.hh"
#include "G4Polyhedra.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4RotationMatrix.hh"
#include "G4Transform3D.hh"
#include "G4SDManager.hh"
#include "G4MultiFunctionalDetector.hh"
#include "G4VPrimitiveScorer.hh"
#include "G4PSEnergyDeposit.hh"
#include "G4PSDoseDeposit.hh"
#include "G4VisAttributes.hh"
#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"
#include "G4SubtractionSolid.hh"
#include "G4VSensitiveDetector.hh"
#include <G4AffineTransform.hh>

// ###################################################################################

SpecMATSimDetectorConstruction::SpecMATSimDetectorConstruction():G4VUserDetectorConstruction(),fCheckOverlaps(true)
{
  //****************************************************************************//
  //********************************* World ************************************//
  //****************************************************************************//
  // Dimensions of world
  // half-size
  worldSizeXY = 40*cm;
  worldSizeZ  = 40*cm;

  // Define world material manually
  N  = new G4Element("Nitrogen", "N", z=7., a=14.01*g/mole);
  O  = new G4Element("Oxygen", "O", z=8., a=16.00*g/mole);
  density = 0.2E-5*mg/cm3;
  Air = new G4Material("Air", density, ncomponents=2);
  Air->AddElement(N, fractionmass=70*perCent);
  Air->AddElement(O, fractionmass=30*perCent);
  // or from the GEANT4 library
  nist = G4NistManager::Instance();
  default_mat = nist->FindOrBuildMaterial("G4_AIR", false);

  solidWorld = new G4Box("World", worldSizeXY, worldSizeXY, worldSizeZ);
  logicWorld = new G4LogicalVolume(solidWorld, Air, "World");
  physWorld = new G4PVPlacement(0, G4ThreeVector(), logicWorld, "World", 0, false, 0, fCheckOverlaps);
  //physWorld = new G4PVPlacement(no rotation, at (0,0,0), its logical volume, its name, its mother  volume, no boolean operation, copy number, checking overlaps);

  //World visual attributes
  worldVisAtt = new G4VisAttributes();					//Instantiation of visualization attributes with blue colour
  worldVisAtt->SetVisibility(false);						//Pass this object to Visualization Manager for visualization
  logicWorld->SetVisAttributes(worldVisAtt);

  //****************************************************************************//
  //******************************* Detector Array *****************************//
  //****************************************************************************//
  // Number of segments and rings in the array
  nbSegments = 15;                //# of detectors in one ring
  nbCrystInSegmentRow = 2;        //# of rings
  nbCrystInSegmentColumn = 1;     //# of detectors in a segment
  gap = 3*mm;                     //distance between rings

  //Optional parts of the TPC, to introduce additional gamma ray attenuation
  //in the materials in between the beam and the detectors
  vacuumChamber = "yes";           //"yes"/"no"
  /*vacuumFlangeSizeX = 150*mm;
  vacuumFlangeSizeY = 29*mm;
  vacuumFlangeSizeZ = 3*mm;
  vacuumFlangeThickFrontOfScint = 3*mm;*/

  vacuumTubeThickness = 3*mm;

  insulationTube = "no";          //"yes"/"no"
  insulationTubeThickness = 3*mm;

  ComptSuppFlag = "no";           //"yes"/"no"

  dPhi = twopi/nbSegments;
  half_dPhi = 0.5*dPhi;
  tandPhi = std::tan(half_dPhi);
  //****************************************************************************//
  //**************** CeBr3 cubic scintillator 1.5"x1.5"x1.5" *******************//
  //****************************************************************************//

  //--------------------------------------------------------//
  //***************** Scintillation crystal ****************//
  //--------------------------------------------------------//
  // Dimensions of the crystal
  sciCrystSizeX = 24.*mm;								//Size and position of all components depends on Crystal size and position.
  sciCrystSizeY = 24.*mm;
  sciCrystSizeZ = 24.*mm;

  // Define Scintillation material and its compounds


  // LaBr3 material
  La = new G4Element("Lanthanum", "La", z=57., a=138.9055*g/mole);
  Br = new G4Element("Bromine", "Br", z=35., a=79.904*g/mole);

  density = 5.1*g/cm3;
  LaBr3 = new G4Material("LaBr3", density, ncomponents=2);
  LaBr3->AddElement (La, natoms=1);
  LaBr3->AddElement (Br, natoms=3);

  // CeBr3 material
  Ce = new G4Element("Cerium", "Ce", z=58., a=140.116*g/mole);

  density = 5.1*g/cm3;
  CeBr3 = new G4Material("CeBr3", density, ncomponents=2);
  CeBr3->AddElement (Ce, natoms=1);
  CeBr3->AddElement (Br, natoms=3);

  sciCrystMat = CeBr3;

  // Position of the crystal
  sciCrystPosX = 0;									//Position of the Crystal along the X axis
  sciCrystPosY = 0;									//Position of the Crystal along the Y axis
  sciCrystPosZ = 0; 			 					//Position of the Crystal along the Z axis

  sciCrystPos = G4ThreeVector(sciCrystPosX, sciCrystPosY, sciCrystPosZ);
  sciCrystSolid = new G4Box("sciCrystSolid", sciCrystSizeX, sciCrystSizeY, sciCrystSizeZ);  // Define box for Crystal
  sciCrystLog = new G4LogicalVolume(sciCrystSolid, sciCrystMat, "crystal");                 // Define Logical Volume for Crystal

  // Visualization attributes for the Crystal logical volume
  sciCrystVisAtt = new G4VisAttributes(G4Colour(0.0, 0.0, 1.0));
  sciCrystVisAtt->SetVisibility(true);
  sciCrystVisAtt->SetForceSolid(true);
  //sciCrystVisAtt->SetForceWireframe(true);
  sciCrystLog->SetVisAttributes(sciCrystVisAtt);

  //--------------------------------------------------------//
  //*********************** Reflector **********************//
  //--------------------------------------------------------//
  // Thickness of reflector walls
  sciReflWallThickX = 0.5*mm;
  sciReflWallThickY = 0.5*mm;
  sciReflWindThick = 0.5*mm;

  // Outer dimensions of the reflector relative to the crystal size
  sciReflSizeX = sciCrystSizeX + sciReflWallThickX;
  sciReflSizeY = sciCrystSizeY + sciReflWallThickY;
  sciReflSizeZ = sciCrystSizeZ + sciReflWindThick/2;

  // Define Reflector (white powder TiO2) material and its compounds
  Ti = new G4Element("Titanium", "Ti", z=22., a=47.9*g/mole);

  density = 4.23*g/cm3;
  TiO2 = new G4Material("TiO2", density, ncomponents=2);
  TiO2->AddElement (Ti, natoms=1);
  TiO2->AddElement (O, natoms=2);

  sciReflMat = TiO2;

  // Position of the reflector relative to the crystal position
  sciReflPosX = sciCrystPosX;
  sciReflPosY = sciCrystPosY;
  sciReflPosZ = sciCrystPosZ - sciReflWindThick/2;					//Position of the Reflector relative to the Al Housing along the Z axis

  sciReflPos = G4ThreeVector(sciReflPosX, sciReflPosY, sciReflPosZ);
  reflBoxSolid = new G4Box("reflBoxSolid", sciReflSizeX, sciReflSizeY, sciReflSizeZ); // Define box for Reflector
  sciReflSolid = new G4SubtractionSolid("sciReflSolid", reflBoxSolid, sciCrystSolid, 0, G4ThreeVector(sciCrystPosX, sciCrystPosY, sciReflWindThick/2)); // Subtracts Crystal box from Reflector box
  sciReflLog = new G4LogicalVolume(sciReflSolid, sciReflMat, "sciReflLog"); // Define Logical Volume for Reflector//

  // Visualization attributes for the Reflector logical volume
  sciReflVisAtt = new G4VisAttributes(G4Colour(1.0, 1.0, 0.0));					//Instantiation of visualization attributes with yellow colour
  sciReflVisAtt->SetVisibility(true);							                      //Pass this object to Visualization Manager for visualization
  sciReflLog->SetVisAttributes(sciReflVisAtt);						              //Assignment of visualization attributes to the logical volume of the Reflector

  //--------------------------------------------------------//
  //******************** Aluminum Housing ******************//
  //--------------------------------------------------------//
  // Dimensions of Housing (half-side)
  sciHousWallThickX = 3.0*mm;
  sciHousWallThickY = 3.0*mm;
  sciHousWindThick = 1.0*mm;


  // Outer dimensions of the housing relative to the crystal size and to the thickness of the reflector
  sciHousSizeX = sciCrystSizeX + sciReflWallThickX + sciHousWallThickX;
  sciHousSizeY = sciCrystSizeY + sciReflWallThickY + sciHousWallThickY;
  sciHousSizeZ = sciCrystSizeZ + sciReflWindThick/2 + sciHousWindThick/2;

  // Define Housing material and its compounds
  Al = new G4Element("Aluminum", "Al", z=13.,	a=26.98*g/mole);

  density = 2.7*g/cm3;
  Al_Alloy = new G4Material("Aluminum_", density, ncomponents=1);
  Al_Alloy->AddElement (Al, natoms=1);

  sciHousMat = Al_Alloy;

  // Position of the housing relative to the crystal position
  sciHousPosX = sciCrystPosX;
  sciHousPosY = sciCrystPosY;
  sciHousPosZ = sciCrystPosZ - (sciReflWindThick/2 + sciHousWindThick/2);

  sciHousPos = G4ThreeVector(sciHousPosX, sciHousPosY, sciHousPosZ);


  housBoxASolid = new G4Box("housBoxASolid", sciHousSizeX, sciHousSizeY, sciHousSizeZ); // Define box for Housing
  sciHousSolid = new G4SubtractionSolid("housBoxBSolid", housBoxASolid, reflBoxSolid, 0, G4ThreeVector(sciReflPosX, sciReflPosY, sciHousWindThick/2)); // Subtracts Reflector box from Housing box
  sciHousLog = new G4LogicalVolume(sciHousSolid, sciHousMat, "sciCaseLog");	//Housing logic volume name

  // Visualization attributes for the Housing logical volume
  sciHousVisAtt =
  new G4VisAttributes(G4Colour(0.5, 0.5, 0.5));
  sciHousVisAtt->SetVisibility(true);
  sciHousLog->SetVisAttributes(sciHousVisAtt);

  //--------------------------------------------------------//
  //******************** Quartz window *********************//
  //--------------------------------------------------------//
  // Dimensions of the Window (half-side)
  sciWindSizeX = sciCrystSizeX + sciReflWallThickX + sciHousWallThickX;						//X half-size of the Window
  sciWindSizeY = sciCrystSizeY + sciReflWallThickY + sciHousWallThickY;						//Y half-size of the Window
  sciWindSizeZ = 1.*mm;									                                          //Z half-size of the Window

  // Define compound elements for Quartz material

  Si = new G4Element("Silicon", "Si", z=14.,	a=28.09*g/mole);

  // Define Quartz material
  density = 2.66*g/cm3;
  Quartz = new G4Material("Quartz", density, ncomponents=2);
  Quartz->AddElement (Si, natoms=1);
  Quartz->AddElement (O, natoms=2);

  sciWindMat = Quartz;

  // Position of the window relative to the crystal
  sciWindPosX = sciCrystPosX ;								                        //Position of the Window along the X axis
  sciWindPosY = sciCrystPosY ;							                         	//Position of the Window along the Y axis
  sciWindPosZ = sciCrystPosZ + sciCrystSizeZ + sciWindSizeZ;	 		  	//Position of the Window relative to the Al Housing along the Z axis

  sciWindPos = G4ThreeVector(sciWindPosX, sciWindPosY, sciWindPosZ);
  sciWindSolid = 	new G4Box("sciWindSolid",	sciWindSizeX, sciWindSizeY, sciWindSizeZ);  // Define solid for the Window
  sciWindLog = new G4LogicalVolume(sciWindSolid, sciWindMat, "sciWindLog");             // Define Logical Volume for Window


  // Visualization attributes for the Window
  sciWindVisAtt = new G4VisAttributes(G4Colour(0.0, 1.0, 1.0));
  sciWindVisAtt->SetVisibility(true);
  sciWindVisAtt->SetForceWireframe(true);
  sciWindLog->SetVisAttributes(sciWindVisAtt);

  //--------------------------------------------------------//
  //******************* Flange material ********************//
  //--------------------------------------------------------//
  vacuumFlangeMat = nist->FindOrBuildMaterial("G4_Al", false);  //Al_Alloy;
  vacuumTubeMat = nist->FindOrBuildMaterial("G4_Al", false);

  /*
  C = new G4Element("Carbon",	"C", z=6., a=12.011*g/mole);
  Mg = new G4Element("Manganese", "Mg", z=25.,	a=54.938044*g/mole);
  Cr = new G4Element("Chromium",	"Cr",	z=24., a=51.9961*g/mole);
  Ni = new G4Element("Nickel",	"Ni",	z=28., a=58.6934*g/mole);
  Mo = new G4Element("Molybdenum",	"Mo",	z=42., a=95.95*g/mole);
  P = new G4Element("Phosphorus", "P",	z=15., a=30.973761998*g/mole);
  S = new G4Element("Sulfur", "S",	z=16., a=32.06*g/mole);
  N = new G4Element("Nitrogen", "N",	z=7.,	a=14.007*g/mole);
  Fe = new G4Element("Iron",	"Fe",	z=26., a=55.845*g/mole);

  density = 8.027*g/cm3;

  Steel_316L = new G4Material("Steel_316L", density, ncomponents=10);

  Steel_316L->AddElement (C, fractionmass=0.030*perCent);
  Steel_316L->AddElement (Mg, fractionmass=2*perCent);
  Steel_316L->AddElement (Si, fractionmass=0.75*perCent);
  Steel_316L->AddElement (Cr, fractionmass=18*perCent);
  Steel_316L->AddElement (Ni, fractionmass=14*perCent);
  Steel_316L->AddElement (Mo, fractionmass=3*perCent);
  Steel_316L->AddElement (P, fractionmass=0.045*perCent);
  Steel_316L->AddElement (S, fractionmass=0.030*perCent);
  Steel_316L->AddElement (N, fractionmass=0.1*perCent);
  Steel_316L->AddElement (Fe, fractionmass=62.045*perCent);
  vacuumFlangeMat = Steel_316L;
  */


  //--------------------------------------------------------//
  //****************** Insulator material ******************//
  //--------------------------------------------------------//
  // Define insulation tube material
  insulationTubeMat = nist->FindOrBuildMaterial("G4_Al", false);
  /*
  H = new G4Element("Hidrogen",	"H", z=1., a=1.008*g/mole);

  density = 0.946*g/cm3;

  Polypropylen_C3H6 = new G4Material("Polypropylen_C3H6", density, ncomponents=2);
  Polypropylen_C3H6->AddElement (C, natoms=3);
  Polypropylen_C3H6->AddElement (H, natoms=6);

  insulationTubeMat = Polypropylen_C3H6;
  */



}

// ###################################################################################

SpecMATSimDetectorConstruction::~SpecMATSimDetectorConstruction()
{
}

// ###################################################################################

void SpecMATSimDetectorConstruction::DefineMaterials()
{
}

// ###################################################################################

G4double SpecMATSimDetectorConstruction::ComputeCircleR1()
{
  if (nbSegments == 1) {
    circleR1 = 150;
  }
  else if (nbSegments == 2) {
    circleR1 = 100;
  }
  else {
    if (vacuumChamber == "yes") {
      /*if (vacuumFlangeSizeY>sciHousSizeY*nbCrystInSegmentColumn) {
        circleR1 = vacuumFlangeSizeY/(tandPhi);
      }
      else {
        circleR1 = sciHousSizeY*nbCrystInSegmentColumn/(tandPhi);
      }*/
      circleR1 = sciHousSizeY*nbCrystInSegmentColumn/(tandPhi);
    }
    else {
      circleR1 = sciHousSizeY*nbCrystInSegmentColumn/(tandPhi);
    }
  }
  return circleR1;
}

// ###################################################################################

G4VPhysicalVolume* SpecMATSimDetectorConstruction::Construct()
{
  //#####################################################################//
  //#### Positioning of scintillation crystals in the detector array ####//
  //#####################################################################//

  circleR1 = SpecMATSimDetectorConstruction::ComputeCircleR1();

  // Define segment which will conain crystals
  nist = G4NistManager::Instance();
  segment_mat = nist->FindOrBuildMaterial("G4_Galactic", false);
  segmentBox = new G4Box("segmentBox", sciHousSizeX*nbCrystInSegmentRow+gap*(nbCrystInSegmentRow-1)/2, sciHousSizeY*nbCrystInSegmentColumn, sciHousSizeZ+sciWindSizeZ);

  // Checking if the flange dimensions are not smaller than the segment dimensions
  if (vacuumFlangeSizeY<sciHousSizeY*nbCrystInSegmentColumn) {
    vacuumFlangeSizeY=sciHousSizeY*nbCrystInSegmentColumn;
  }

  //--------------------------------------------------------//
  //****************** Compton Suppressor ******************//
  //--------------------------------------------------------//
  if (ComptSuppFlag == "yes") {
    Bi = new G4Element("Bismuth",	"Bi",	z=83., a=208.98*g/mole);
    Ge = new G4Element("Germanium",	"Ge",	z=32., a=72.63*g/mole);
    O = new G4Element("Oxygen",	"O", z=8., a=15.99*g/mole);

    density = 7.13*g/cm3;
    BGO = new G4Material("BGO", density, ncomponents=3);
    BGO->AddElement (Bi, natoms=4);
    BGO->AddElement (Ge, natoms=3);
    BGO->AddElement (O, natoms=12);

    ComptSuppMat = BGO;

    ComptSuppSizeX = 24.*mm;						//Size and position of all components depends on Crystal size and position.
    ComptSuppSizeY = 108.*mm;
    ComptSuppSizeZ = sciHousSizeX*3*mm + (gap/2)*2*mm;

    // Position of the Compton Suppressor
    ComptSuppPosX = 0;									//Position of the Compton Suppressor along the X axis
    ComptSuppPosY = 0;									//Position of the Compton Suppressor along the Y axis
    ComptSuppPosZ = -300; 			 				//Position of the Compton Suppressor along the Z axis

    ComptSuppPos = G4ThreeVector(ComptSuppPosX, ComptSuppPosY, ComptSuppPosZ);
    ComptSuppSolidBox = new G4Box("ComptSuppSolid", 117/2, 30, ComptSuppSizeZ);
    ComptSuppSolidBoxUp = new G4Box("ComptSuppSolidUp", 200, 30*std::cos(dPhi/2), 2*ComptSuppSizeZ);

    ComptSuppRotmBoxUp  = G4RotationMatrix();               //** rotation matrix for positioning ComptSupp
    ComptSuppRotmBoxUp.rotateZ(dPhi/2);                                      //** rotation matrix for positioning ComptSupp
    positionComptSuppBoxUp = G4ThreeVector(-117/2, 30, 0);
    transformComptSuppBoxUp = G4Transform3D(ComptSuppRotmBoxUp,positionComptSuppBoxUp);


    ComptSuppSolidBoxDown = new G4Box("ComptSuppSolidDown", 200, 30*std::cos(dPhi/2), 2*ComptSuppSizeZ);

    ComptSuppRotmBoxDown  = G4RotationMatrix();              //** rotation matrix for positioning ComptSupp
    ComptSuppRotmBoxDown.rotateZ(-dPhi/2);                                    //** rotation matrix for positioning ComptSupp
    positionComptSuppBoxDown = G4ThreeVector(-117/2, -30, 0);
    transformComptSuppBoxDown = G4Transform3D(ComptSuppRotmBoxDown,positionComptSuppBoxDown);


    ComptSuppSolidBoxWithoutUp = new G4SubtractionSolid("ComptSuppSolidBoxWithoutUp", ComptSuppSolidBox, ComptSuppSolidBoxUp, transformComptSuppBoxUp);
    ComptSuppSolidBoxWithoutDown = new G4SubtractionSolid("ComptSuppSolidBoxWithoutDown", ComptSuppSolidBoxWithoutUp, ComptSuppSolidBoxDown, transformComptSuppBoxDown);
    rotationAngle=dPhi/2;

    ComptSuppTrapLog = new G4LogicalVolume(ComptSuppSolidBoxWithoutDown, ComptSuppMat, "ComptSuppTrap");
    for (G4int i = 0; i < nbSegments; i++) {
      ComptSuppTrapRotm  = G4RotationMatrix();               //** rotation matrix for positioning ComptSupp
      ComptSuppTrapRotm.rotateZ(rotationAngle);                               //** rotation matrix for positioning ComptSupp
      positionComptSuppTrap = G4ThreeVector((117/2+circleR1/std::cos(dPhi/2))*std::cos(rotationAngle), (117/2+circleR1/std::cos(dPhi/2))*std::sin(rotationAngle), 0);
      transformComptSuppTrap = G4Transform3D(ComptSuppTrapRotm,positionComptSuppTrap);
      new G4PVPlacement(transformComptSuppTrap, ComptSuppTrapLog, "ComptSuppTrapPl", logicWorld, false, 100+i, fCheckOverlaps);
      rotationAngle += dPhi;
    }

    // Visualization attributes for the Compton Suppressor logical volume
    ComptSuppVisAtt = new G4VisAttributes(G4Colour(1.0, 0.0, 0.0));
    ComptSuppVisAtt->SetVisibility(true);
    ComptSuppVisAtt->SetForceSolid(true);
    ComptSuppTrapLog->SetVisAttributes(ComptSuppVisAtt);

  }

  //Define the vacuum chamber flange
  if (vacuumChamber == "yes") {
    /*vacuumFlangeBox = new G4Box("vacuumFlangeBox", vacuumFlangeSizeX,	vacuumFlangeSizeY, vacuumFlangeSizeZ);
    // Subtracts Reflector box from Housing box
    vacuumFlangeSolid = new G4SubtractionSolid("vacuumFlangeSolid", vacuumFlangeBox, segmentBox, 0, G4ThreeVector(0, 0, (sciHousSizeZ+sciWindSizeZ)+vacuumFlangeSizeZ-(2*vacuumFlangeSizeZ-vacuumFlangeThickFrontOfScint)));
    vacuumFlangeBoxLog = new G4LogicalVolume(vacuumFlangeSolid, vacuumFlangeMat, "vacuumFlangeBoxLog");

    vacuumSideFlangeMat = Al_Alloy;
    rotSideFlnge  = G4RotationMatrix();
    rotSideFlnge.rotateZ(dPhi/2);
    positionSideFlange1 = G4ThreeVector(0, 0, vacuumFlangeSizeX);
    transformSideFlange1 = G4Transform3D(rotSideFlnge, positionSideFlange1);
    positionSideFlange2 = G4ThreeVector(0, 0, -vacuumFlangeSizeX-2*vacuumFlangeSizeZ);
    transformSideFlange2 = G4Transform3D(rotSideFlnge, positionSideFlange2);
    G4double vacuumChamberSideFlangeThickness[] = {0, 2*vacuumFlangeSizeZ};
    G4double vacuumChamberSideFlangeInnerR[] = {0, 0};
    G4double vacuumChamberSideFlangeOuterR[] = {circleR1+2*vacuumFlangeSizeZ, circleR1+2*vacuumFlangeSizeZ};

    vacuumChamberSideFlange = new G4Polyhedra("vacuumChamberSideFlange", 0, 2*3.1415926535897932384626433, nbSegments, 2, vacuumChamberSideFlangeThickness, vacuumChamberSideFlangeInnerR, vacuumChamberSideFlangeOuterR);
    vacuumChamberSideFlangeLog = new G4LogicalVolume(vacuumChamberSideFlange, vacuumSideFlangeMat, "vacuumChamberSideFlangeLog");
    new G4PVPlacement(transformSideFlange1, vacuumChamberSideFlangeLog, "VacuumChamberSideFlangeLog", logicWorld, false, 1, fCheckOverlaps);
    new G4PVPlacement(transformSideFlange2, vacuumChamberSideFlangeLog, "VacuumChamberSideFlangeLog", logicWorld, false, 2, fCheckOverlaps);*/

    vacuumTubeInnerRadius = circleR1-vacuumTubeThickness;
    vacuumTubeOuterRadius = circleR1;
    vacuumTubeSolid = new G4Tubs("vacuumTubeSolid",	vacuumTubeInnerRadius, vacuumTubeOuterRadius,	102.25*mm, 0*deg, 360*deg);
    vacuumTubeLog = new G4LogicalVolume(vacuumTubeSolid, vacuumTubeMat, "vacuumTubeLog");
    new G4PVPlacement(0, G4ThreeVector(0,0,29.25*mm), vacuumTubeLog, "vacuumTubePhys", logicWorld, false, 1, fCheckOverlaps);

    // Visualization attributes for the insulation tube
    vacuumTubeVisAtt = new G4VisAttributes(G4Colour(0.5, 0.5, 0.5));
    vacuumTubeVisAtt->SetVisibility(true);
    vacuumTubeVisAtt->SetForceSolid(true);
    vacuumTubeLog->SetVisAttributes(vacuumTubeVisAtt);

    //Secondtube
    vacuumTubeInnerRadius2 = vacuumTubeInnerRadius;
    vacuumTubeOuterRadius2 = 226*mm;
    vacuumTubeSolid2 = new G4Tubs("vacuumTubeSolid2",	vacuumTubeInnerRadius2, vacuumTubeOuterRadius2,	5*mm, 0*deg, 360*deg);
    vacuumTubeLog2 = new G4LogicalVolume(vacuumTubeSolid2, vacuumTubeMat, "vacuumTubeLog2");
    new G4PVPlacement(0, G4ThreeVector(0,0,-78*mm), vacuumTubeLog2, "vacuumTubePhys2", logicWorld, false, 1, fCheckOverlaps);

    // Visualization attributes for the insulation tube
    vacuumTubeLog2->SetVisAttributes(vacuumTubeVisAtt);

    //Thirdtube
    vacuumTubeInnerRadius3 = 150*mm;
    vacuumTubeOuterRadius3 = 255*mm;
    vacuumTubeSolid3 = new G4Tubs("vacuumTubeSolid3",	vacuumTubeInnerRadius3, vacuumTubeOuterRadius3,	5*mm, 0*deg, 360*deg);
    vacuumTubeLog3 = new G4LogicalVolume(vacuumTubeSolid3, vacuumTubeMat, "vacuumTubeLog3");
    new G4PVPlacement(0, G4ThreeVector(0,0,-88*mm), vacuumTubeLog3, "vacuumTubePhys3", logicWorld, false, 1, fCheckOverlaps);

    // Visualization attributes for the insulation tube
    vacuumTubeLog3->SetVisAttributes(vacuumTubeVisAtt);

    //Fourthtube
    vacuumTubeInnerRadius4 = 200*mm;
    vacuumTubeOuterRadius4 = 255*mm;
    vacuumTubeSolid4 = new G4Tubs("vacuumTubeSolid4",	vacuumTubeInnerRadius4, vacuumTubeOuterRadius4,	15*mm, 0*deg, 360*deg);
    vacuumTubeLog4 = new G4LogicalVolume(vacuumTubeSolid4, vacuumTubeMat, "vacuumTubeLog4");
    new G4PVPlacement(0, G4ThreeVector(0,0,-108*mm), vacuumTubeLog4, "vacuumTubePhys4", logicWorld, false, 1, fCheckOverlaps);

    // Visualization attributes for the insulation tube
    vacuumTubeLog4->SetVisAttributes(vacuumTubeVisAtt);

    //Fifthtube
    vacuumTubeInnerRadius5 = vacuumTubeInnerRadius*mm;
    vacuumTubeOuterRadius5 = 254*mm;
    vacuumTubeSolid5 = new G4Tubs("vacuumTubeSolid5",	vacuumTubeInnerRadius5, vacuumTubeOuterRadius5,	7.5*mm, 0*deg, 360*deg);
    vacuumTubeLog5 = new G4LogicalVolume(vacuumTubeSolid5, vacuumTubeMat, "vacuumTubeLog5");
    new G4PVPlacement(0, G4ThreeVector(0,0,138*mm), vacuumTubeLog5, "vacuumTubePhys5", logicWorld, false, 1, fCheckOverlaps);

    // Visualization attributes for the insulation tube
    vacuumTubeLog5->SetVisAttributes(vacuumTubeVisAtt);

    //Sixthtube
    vacuumTubeInnerRadius6 = 239*mm;
    vacuumTubeOuterRadius6 = 254*mm;
    vacuumTubeSolid6 = new G4Tubs("vacuumTubeSolid6",	vacuumTubeInnerRadius6, vacuumTubeOuterRadius6,	37.5*mm, 0*deg, 360*deg);
    vacuumTubeLog6 = new G4LogicalVolume(vacuumTubeSolid6, vacuumTubeMat, "vacuumTubeLog6");
    new G4PVPlacement(0, G4ThreeVector(0,0,174*mm), vacuumTubeLog6, "vacuumTubePhys6", logicWorld, false, 1, fCheckOverlaps);

    // Visualization attributes for the insulation tube
    vacuumTubeLog6->SetVisAttributes(vacuumTubeVisAtt);

    //Seventhtube
    vacuumTubeInnerRadius7 = 239*mm;
    vacuumTubeOuterRadius7 = 305*mm;
    vacuumTubeSolid7 = new G4Tubs("vacuumTubeSolid7",	vacuumTubeInnerRadius7, vacuumTubeOuterRadius7,	10*mm, 0*deg, 360*deg);
    vacuumTubeLog7 = new G4LogicalVolume(vacuumTubeSolid7, vacuumTubeMat, "vacuumTubeLog7");
    new G4PVPlacement(0, G4ThreeVector(0,0,226.5*mm), vacuumTubeLog7, "vacuumTubePhys7", logicWorld, false, 1, fCheckOverlaps);

    // Visualization attributes for the insulation tube
    vacuumTubeLog7->SetVisAttributes(vacuumTubeVisAtt);

  }

  //Defines insulation tube between the field cage and the vacuum chamber which might be used for preventing sparks in the real setup
  //And its stopping power should be simulated
  //
  if (/*vacuumChamber == "yes" &&*/ insulationTube == "yes") {
    //Geometry of the insulation Tube
    insulationTubeInnerRadius = circleR1-insulationTubeThickness;
    insulationTubeOuterRadius = circleR1;
    insulationTubeSolid = new G4Tubs("insulationTubeSolid",	insulationTubeInnerRadius, insulationTubeOuterRadius,	150*mm, 0*deg, 360*deg);
    insulationTubeLog = new G4LogicalVolume(insulationTubeSolid, insulationTubeMat, "insulationTubeLog");
    new G4PVPlacement(0, G4ThreeVector(0,0,0), insulationTubeLog, "insulationTubePhys", logicWorld, false, 1, fCheckOverlaps);

    // Visualization attributes for the insulation tube
    insulationTubeVisAtt = new G4VisAttributes(G4Colour(0.45, 0.25, 0.0));
    insulationTubeVisAtt->SetVisibility(true);
    insulationTubeVisAtt->SetForceSolid(true);
    insulationTubeLog->SetVisAttributes(insulationTubeVisAtt);
  }

  //Positioning of segments and crystals in the segment

  //In TotalCrystNb array will be stored coordinates of the all crystals, which could be used for further Doppler correction
  TotalCrystNb = nbCrystInSegmentRow*nbCrystInSegmentColumn*nbSegments;   //Dimension of the dynamic the array
  crystalPositionsArray = new G4ThreeVector[TotalCrystNb];                //Dinamic mamory allocation for the array
  for (i=0; i<TotalCrystNb; i++) {
    crystalPositionsArray[i] = G4ThreeVector(0.,0.,0.);                   // Initialize all elements of the array to zero.
  }

  i = 0;          //counter for reconstruction of crystal positions
  crysNb = 1;     //crystal counter
  for (iseg = 0; iseg < nbSegments ; iseg++) {
    phi = iseg*dPhi;
    rotm  = G4RotationMatrix();                      //** rotation matrix for positioning segments
    rotm.rotateY(90*deg);                            //** rotation matrix for positioning segments
    rotm.rotateZ(phi);                               //** rotation matrix for positioning segments

    rotm2  = G4RotationMatrix();                     //### rotation matrix for reconstruction of crystal positions
    rotm2.rotateX(360*deg - phi);                    //### rotation matrix for reconstruction of crystal positions
    rotm3  = G4RotationMatrix();                     //### rotation matrix for reconstruction of crystal positions
    rotm3.rotateY(90*deg);                           //### rotation matrix for reconstruction of crystal positions

    uz = G4ThreeVector(std::cos(phi), std::sin(phi), 0.); //cooficient which will be used for preliminary rotation of the segments and crystals
    segmentBoxLog = new G4LogicalVolume(segmentBox, segment_mat, "segmentBoxLog");
    positionInSegment = G4ThreeVector(-(nbCrystInSegmentRow*sciHousSizeX+gap*(nbCrystInSegmentRow-1)/2-sciHousSizeX), -(nbCrystInSegmentColumn*sciHousSizeY-sciHousSizeY), (sciHousSizeZ-sciCrystSizeZ-sciWindSizeZ));

    for (icrystRow = 0; icrystRow < nbCrystInSegmentColumn; icrystRow++) {
      for (icrystCol = 0; icrystCol < nbCrystInSegmentRow; icrystCol++) {
        rotm1  = G4RotationMatrix();

        positionCryst = (G4ThreeVector(0., 0., sciCrystPosZ) + positionInSegment);
        positionWind = (G4ThreeVector(0., 0., sciWindPosZ) + positionInSegment);
        positionRefl = (G4ThreeVector(0., 0., sciReflPosZ) + positionInSegment);
        positionHous = (G4ThreeVector(0., 0., sciHousPosZ) + positionInSegment);

        crystalPositionsArray[crysNb - 1] = positionCryst; //assigning initial crystal positions in a segment into array

        transformCryst = G4Transform3D(rotm1,positionCryst);
        transformWind = G4Transform3D(rotm1,positionWind);
        transformRefl = G4Transform3D(rotm1,positionRefl);
        transformHous = G4Transform3D(rotm1,positionHous);

        // Crystal position
        new G4PVPlacement(transformCryst,			//no rotation here rotm1 is empty, position
          sciCrystLog,                        //its logical volume
          "sciCrystPl",                       //its name
          segmentBoxLog,                      //its mother  volume
          false,                              //no boolean operation
          crysNb,                             //crystal unique number will
          fCheckOverlaps);                    // checking overlaps

        new G4PVPlacement(transformWind,
          sciWindLog,
          "sciWindPl",
          segmentBoxLog,
          false,
          crysNb,
          fCheckOverlaps);

        new G4PVPlacement(transformRefl,
          sciReflLog,
          "sciReflPl",
          segmentBoxLog,
          false,
          crysNb,
          fCheckOverlaps);

        new G4PVPlacement(transformHous,
          sciHousLog,
          "sciHousPl",
          segmentBoxLog,
          false,
          crysNb,
          fCheckOverlaps);

        crysNb += 1;
        positionInSegment += G4ThreeVector(sciHousSizeX*2+gap, 0., 0.);
      }
      positionInSegment -= G4ThreeVector(nbCrystInSegmentRow*sciHousSizeX*2+gap*(nbCrystInSegmentRow), 0., 0.);
      positionInSegment += G4ThreeVector(0., sciHousSizeY*2, 0.);
    }

    //segment and flange positioning
    /*if (vacuumChamber == "yes") {
      //Flange positioning
      positionVacuumFlange = (circleR1+vacuumFlangeSizeZ)*uz;
      transformVacuumFlange = G4Transform3D(rotm, positionVacuumFlange);

      new G4PVPlacement(transformVacuumFlange,                            //position
        vacuumFlangeBoxLog,                                               //its logical volume
        "VacuumFlange",                                                   //its name
        logicWorld,                                                       //its mother  volume
        false,                                                            //no boolean operation
        iseg,                                                             //copy number
        fCheckOverlaps);                                                  // checking overlaps

      //Segment positioning
      positionSegment = (circleR1+2*vacuumFlangeSizeZ+(sciHousSizeZ+sciWindSizeZ)-(2*vacuumFlangeSizeZ-vacuumFlangeThickFrontOfScint))*uz;
      transformSegment = G4Transform3D(rotm, positionSegment);

      new G4PVPlacement(transformSegment,                                 //position
        segmentBoxLog,                                                    //its logical volume
        "Segment",                                                        //its name
        logicWorld,                                                       //its mother  volume
        false,                                                            //no boolean operation
        iseg,                                                             //copy number
        fCheckOverlaps);                                                  // checking overlaps

      //Saving crystal positions in the crystalPositionsArray array
      for (; i < crysNb-1; i++) {
        TransformCrystPos1.SetNetRotation(rotm2); //rotates the crystal centers (in one segment) by angle phi around X
        crystalPositionsArray[i] = TransformCrystPos1.TransformPoint(crystalPositionsArray[i]);

        TransformCrystPos.SetNetRotation(rotm3); //rotates the crystal centers (in one segment) by 90deg around Y
        TransformCrystPos.SetNetTranslation(positionSegment);
        crystalPositionsArray[i] = TransformCrystPos.TransformPoint(crystalPositionsArray[i]);
      }
    }*/
    //segment position in case vacuumChamber is "no"
    //else {
      //Segment positioning
      positionSegment = (circleR1+(sciHousSizeZ+sciWindSizeZ))*uz;
      transformSegment = G4Transform3D(rotm, positionSegment);

      new G4PVPlacement(transformSegment,                                 //position
        segmentBoxLog,                                                    //its logical volume
        "Segment",                                                        //its name
        logicWorld,                                                       //its mother  volume
        false,                                                            //no boolean operation
        iseg,                                                             //copy number
        fCheckOverlaps);                                                  // checking overlaps

      //Saving crystal positions in the crystalPositionsArray array
      for (; i < crysNb-1; i++) {
        TransformCrystPos1.SetNetRotation(rotm2); //rotates the crystal centers (in one segment) by angle phi around X
        crystalPositionsArray[i] = TransformCrystPos1.TransformPoint(crystalPositionsArray[i]);

        TransformCrystPos.SetNetRotation(rotm3); //rotates the crystal centers (in one segment) by 90deg around Y
        TransformCrystPos.SetNetTranslation(positionSegment);
        crystalPositionsArray[i] = TransformCrystPos.TransformPoint(crystalPositionsArray[i]);
      }
    //}
  }

  // Prints dimensions of the scintillation array
  G4cout <<""<< G4endl;
  G4cout <<"$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$"<< G4endl;
  G4cout <<"$$$$"<< G4endl;
  G4cout <<"$$$$"<<" Crystal material: "<<sciCrystMat->GetName()<< G4endl;
  G4cout <<"$$$$"<<" Reflector material: "<<sciReflMat->GetName()<< G4endl;
  G4cout <<"$$$$"<<" Housing material: "<<sciHousMat->GetName()<< G4endl;
  G4cout <<"$$$$"<<" Optic window material: "<<sciWindMat->GetName()<< G4endl;
  G4cout <<"$$$$"<< G4endl;
  G4cout <<"$$$$"<<" Single crystal dimensions: "<<sciCrystSizeX*2<<"mmx"<<sciCrystSizeY*2<<"mmx"<<sciCrystSizeZ*2<<"mm "<< G4endl;
  G4cout <<"$$$$"<<" Dimensions of the crystal housing: "<<sciHousSizeX*2<<"mmx"<<sciHousSizeY*2<<"mmx"<<sciHousSizeZ*2<<"mm "<< G4endl;
  G4cout <<"$$$$"<<" Housing wall thickness: "<<sciHousWallThickX<<"mm "<< G4endl;
  G4cout <<"$$$$"<<" Housing window thickness: "<<sciHousWindThick<<"mm "<< G4endl;
  G4cout <<"$$$$"<<" Reflecting material wall thickness: "<<sciReflWallThickX<<"mm "<< G4endl;
  G4cout <<"$$$$"<<" Reflecting material thickness in front of the window: "<<sciReflWindThick<<"mm "<< G4endl;
  G4cout <<"$$$$"<< G4endl;
  G4cout <<"$$$$"<<" Number of segments in the array: "<<nbSegments<<" "<< G4endl;
  G4cout <<"$$$$"<<" Number of crystals in the segment row: "<<nbCrystInSegmentRow<<" "<< G4endl;
  G4cout <<"$$$$"<<" Number of crystals in the segment column: "<<nbCrystInSegmentColumn<<" "<< G4endl;
  G4cout <<"$$$$"<<" Number of crystals in the array: "<<nbSegments*nbCrystInSegmentRow*nbCrystInSegmentColumn<<" "<< G4endl;
  G4cout <<"$$$$"<<" Segment width: "<<sciHousSizeY*nbCrystInSegmentColumn*2<<"mm "<< G4endl;
  G4cout <<"$$$$"<< G4endl;
  G4cout <<"$$$$"<<" Radius of a circle inscribed in the array: "<<circleR1<<"mm "<< G4endl;
  G4cout <<"$$$$"<< G4endl;
  if (vacuumChamber == "yes") {
    /*G4cout <<"$$$$"<<" Flange material: "<<vacuumFlangeMat->GetName()<< G4endl;
    G4cout <<"$$$$"<<" SideFlange material: "<<vacuumSideFlangeMat->GetName()<< G4endl;
    G4cout <<"$$$$"<<" Flange width: "<<vacuumFlangeSizeY*2<<"mm "<< G4endl;
    G4cout <<"$$$$"<<" Flange thickness: "<<vacuumFlangeSizeZ*2<<"mm "<< G4endl;
    G4cout <<"$$$$"<<" Flange thickness in front of the window: "<<vacuumFlangeThickFrontOfScint<<"mm "<< G4endl;*/
  }
  G4cout <<"$$$$"<< G4endl;
  if (/*vacuumChamber == "yes" &&*/ insulationTube == "yes") {
    G4cout <<"$$$$"<<" Insulator material: "<<insulationTubeMat->GetName()<< G4endl;
    G4cout <<"$$$$"<<" Insulator thickness: "<<insulationTubeThickness<<"mm "<< G4endl;
    G4cout <<"$$$$"<<" Insulator tube outer radius: "<<insulationTubeOuterRadius<<"mm "<< G4endl;
    G4cout <<"$$$$"<<" Insulator tube inner radius: "<<insulationTubeInnerRadius<<"mm "<< G4endl;
  }
  G4cout <<"$$$$"<< G4endl;
  G4cout <<"$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$"<< G4endl;
  G4cout <<""<< G4endl;
  G4cout <<"Positions of the crystal centers in the world:"<< G4endl;
  for (i = 0; i < TotalCrystNb; i++) {
    G4cout << "CrystNb" << i+1 << ": " << crystalPositionsArray[i] << G4endl;
  }
  G4cout <<""<< G4endl;
  delete [] crystalPositionsArray; //Free memory allocated for the crystalPositionsArray array
  crystalPositionsArray = NULL;    //Clear a to prevent using invalid memory reference

  CreateScorers();

  return physWorld;
}

// ###################################################################################

void SpecMATSimDetectorConstruction::CreateScorers()
{

  SDman = G4SDManager::GetSDMpointer();
  SDman->SetVerboseLevel(1);

  // declare crystal as a MultiFunctionalDetector scorer
  //
  cryst = new G4MultiFunctionalDetector("crystal");
  primitiv = new G4PSEnergyDeposit("edep");
  cryst->RegisterPrimitive(primitiv);
  SDman->AddNewDetector(cryst);
  sciCrystLog->SetSensitiveDetector(cryst);

  if (ComptSuppFlag == "yes") {
      ComptSupp = new G4MultiFunctionalDetector("ComptSupp");
      ComptSuppPrimitiv = new G4PSEnergyDeposit("edep");
      ComptSupp->RegisterPrimitive(ComptSuppPrimitiv);
      SDman->AddNewDetector(ComptSupp);
      ComptSuppTrapLog->SetSensitiveDetector(ComptSupp);
  }
}

// ###################################################################################
