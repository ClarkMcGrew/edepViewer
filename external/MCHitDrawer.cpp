//File: MCHitDrawer.cpp
//Brief: An MCHitDrawer is an ExternalDrawer for my edepsim display that draws MCHits from this package.  
//Author: Andrew Olivier aolivier@ur.rochester.edu

//plugin includes
#include "plugins/Factory.cpp"

//edepsim includes
#include "TG4Event.h"

//local includes
#include "external/MCHitDrawer.h"

//gl includes
#include "gl/model/PolyMesh.h"

//ROOT includes
#include "TGeoBBox.h" //Easiest way to specify vertices to PolyMesh.

namespace mygl
{
  MCHitDrawer::MCHitDrawer(const tinyxml2::XMLElement* config): ExternalDrawer(), fHits(nullptr), fPalette(0., 1e3), fHitName("NeutronHits")
  {
    const auto hitName = config->Attribute("HitName");
    if(hitName != nullptr) fHitName = hitName;
  }

  void MCHitDrawer::ConnectTree(TTreeReader& reader)
  {
    std::cout << "Connecting MCHitsDrawer to TTreeReader.\n";
    fHits.reset(new TTreeReaderArray<pers::MCHit>(reader, fHitName.c_str())); //TODO: Get name of hits to process from XML file in constructor
  }

  void MCHitDrawer::doRequestScenes(mygl::Viewer& viewer)
  {
    auto& hitTree = viewer.MakeScene("MCHits", fHitRecord, "/home/aolivier/app/evd/src/gl/shaders/colorPerVertex.frag", "/home/aolivier/app/evd/src/gl/shaders/colorPerVertex.vert", "/home/aolivier/app/evd/src/gl/shaders/triangleBorder.geom");
    hitTree.append_column("Energy", fHitRecord.fEnergy);
    hitTree.append_column("Time", fHitRecord.fTime);
    hitTree.append_column("Cause", fHitRecord.fParticle);
  }

  void MCHitDrawer::doDrawEvent(const TG4Event& event, mygl::Viewer& viewer, mygl::VisID& nextID, draw::Services& services)
  {
    viewer.RemoveAll("MCHits");

    auto top = *(viewer.GetScenes().find("MCHits")->second.NewTopLevelNode());
    top[fHitRecord.fEnergy] = std::accumulate(fHits->begin(), fHits->end(), 0., [](double value, const auto& hit) { return value + hit.Energy; });
    top[fHitRecord.fParticle] = fHitName; //Algorithm name
    top[fHitRecord.fTime] = 0.;

    for(const auto& hit: *fHits)
    {
      TGeoBBox box(hit.Width/2., hit.Width/2., hit.Width/2.);
           
      glm::mat4 pos = glm::translate(glm::mat4(), glm::vec3(hit.Position.X(), hit.Position.Y(), hit.Position.Z()));

      auto row = viewer.AddDrawable<mygl::PolyMesh>("MCHits", nextID++, top, true, pos,
                                                    &box, glm::vec4(fPalette(hit.Energy), 1.0)); 
      row[fHitRecord.fEnergy] = hit.Energy;
      row[fHitRecord.fTime] = hit.Position.T();
      row[fHitRecord.fParticle] = std::accumulate(hit.TrackIDs.begin(), hit.TrackIDs.end(), std::string(""), 
                                                  [&event](std::string& names, const int id)
                                                  {
                                                    return names+" "+event.Trajectories[id].Name;
                                                  });
    }
  }

  REGISTER_PLUGIN(MCHitDrawer, draw::ExternalDrawer); 
}
