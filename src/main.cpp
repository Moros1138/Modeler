#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

#define GFX_BLACKBOX_IMPLEMENTATION
#include "gfxBlackbox.h"

#include "portable-file-dialogs.h"

typedef std::vector<olc::vf2d> Model;

class Modeler : public olc::PixelGameEngine
{
public:
    Modeler()
    {
        sAppName = "Modeler";
    }

public:
    
    bool OnUserCreate() override
    {
        // Check that a backend is available
        if (!pfd::settings::available())
        {
            std::cout << "Portable File Dialogs are not available on this platform.\n";
            return false;
        }

        // scaling
        fScale = 22.0f;
        fMinScale = 4.0f;
        fMaxScale = 50.0f;

        // precalculate the screen
        vCenter = {ScreenWidth() / 2, ScreenHeight() / 2};
        vOffset = {0,0};

        // colors
        cBackground =    olc::VERY_DARK_BLUE;
        cGrid =          olc::DARK_BLUE;
        cOrigin =        olc::BLUE;

        // drawing states
        bFill =     true;
        bPoints =   true;
        bStroke =   true;
        bWire =     true;

        // point selection
        selected = -1;

        return true;
    }
    
    olc::vi2d vMouse1;
    olc::vi2d vMouse2;
    olc::vf2d vOffset;
    
    bool OnUserUpdate(float fElapsedTime) override
    {
        int nGridLines = 40;
        std::stringstream sBuf;

        olc::vf2d vMouse = {
            (float)round(((float)GetMouseX() + (float)vCenter.x - (float)ScreenWidth()) / fScale),
            (float)round(((float)GetMouseY() + (float)vCenter.y - (float)ScreenHeight()) / fScale)
        };
        
        sBuf << (vMouse * 0.1f).x << " " << (vMouse * 0.1f).y;

        Model transformed = gfx_blackbox::Polygon::Transform(mModel, vCenter, fScale, 0.0f);
        
        if(GetKey(olc::K1).bPressed)
        {
            auto destination = pfd::save_file("Select a file", ".model", {"Model Files", "*.model"}).result();
            if (!destination.empty())
            {
                SaveFile(destination);
            }
        }

        // Left Mouse Press
        if(GetMouse(0).bPressed)
        {
            selected = -1;
            for(int i = 0; i < mModel.size(); i++)
            {
                if((vMouse-mModel[i]).mag() < 0.1f)
                {
                    selected = i;
                    break;
                }
            }
            
            if(selected == -1)
                mModel.push_back(vMouse);
        }

        // Left Mouse Release
        if(GetMouse(0).bReleased)
        {
            if(selected != -1)
            {
                mModel[selected] = vMouse;
                selected = -1;
            }
        }

        if(GetMouse(2).bPressed)
            vMouse1 = GetMousePos();

        if(GetMouse(2).bHeld)
        {
            vMouse2 = GetMousePos();
            vCenter += (vMouse2 - vMouse1);
            vOffset += (vMouse2 - vMouse1) / fScale;
            vMouse1 = vMouse2;
        }

        // Zoom In
        if(GetMouseWheel() > 0)
        {
            fScale += 1.0f;
            if(fScale > fMaxScale) fScale = fMaxScale;
        }
        
        // Zoom Out
        if(GetMouseWheel() < 0)
        {
            fScale -= 1.0f;
            if(fScale < fMinScale) fScale = fMinScale;
        }
        
        if(GetKey(olc::F).bPressed) bFill = !bFill;
        if(GetKey(olc::P).bPressed) bPoints = !bPoints;
        if(GetKey(olc::S).bPressed) bStroke = !bStroke;
        if(GetKey(olc::W).bPressed) bWire = !bWire;

        if(GetKey(olc::R).bPressed && selected != -1)
        {
            mModel.erase(mModel.begin()+selected);
            selected = -1;
        }

        // DRAWING ROUTINES
        Clear(cBackground);

        // Draw the grid lines      
        for(int i = 0; i < nGridLines; i++)
        {
            if(fScale < 10.0f && i % 2 == 1)
                continue;

            DrawLine(vCenter.x - ((nGridLines-1) * fScale), vCenter.y - (i * fScale), vCenter.x + ((nGridLines-1) * fScale), vCenter.y - (i * fScale), cGrid);
            DrawLine(vCenter.x - ((nGridLines-1) * fScale), vCenter.y + (i * fScale), vCenter.x + ((nGridLines-1) * fScale), vCenter.y + (i * fScale), cGrid);
            DrawLine(vCenter.x - (i * fScale), vCenter.y - ((nGridLines-1) * fScale), vCenter.x - (i * fScale), vCenter.y + ((nGridLines-1) * fScale), cGrid);
            DrawLine(vCenter.x + (i * fScale), vCenter.y - ((nGridLines-1) * fScale), vCenter.x + (i * fScale), vCenter.y + ((nGridLines-1) * fScale), cGrid);
        }
        
        // Draw Grid Origin
        DrawLine(vCenter.x, 0, vCenter.x, ScreenHeight(), cOrigin);
        DrawLine(0, vCenter.y, ScreenWidth(), vCenter.y, cOrigin);

        if(bFill)
            gfx_blackbox::Polygon::Fill(this, transformed, olc::DARK_GREY);
        
        if(bWire)
            gfx_blackbox::Polygon::Wire(this, transformed, olc::GREY);
        
        if(bStroke)
            gfx_blackbox::Polygon::Stroke(this, transformed, olc::WHITE);
        
        if(bPoints)
        {
            int counter = 0;
            for(auto& p : transformed)
            {
                FillCircle(p, 2, olc::RED);
                DrawStringDecal(p+olc::vf2d(1,1), std::to_string(counter), olc::BLACK);
                DrawStringDecal(p, std::to_string(counter), olc::WHITE);
                counter++;
            }
        }

        // Draw the current mouse position if we are not selecting one
        if(selected == -1)
            FillCircle((vCenter + olc::vf2d{vOffset.x, vOffset.y} * fScale) + (vMouse * fScale), 2, olc::YELLOW);
        else
            FillCircle(vCenter + (vMouse * fScale), 2, olc::GREEN);

        // Display current coordinates
        DrawString(5, 5, sBuf.str(), olc::WHITE);
        
        int insX = 5;
        int insY = ScreenHeight() - 14;

        // Instructions
        DrawString(insX, insY, "F)ill P)oints S)troke W)ire", olc::WHITE);
        
        if(bFill)
            DrawString(insX, insY, "F", olc::RED);
        
        insX += GetTextSize("F)ill ").x;
        if(bPoints)
            DrawString(insX, insY, "P", olc::RED);

        insX += GetTextSize("P)oints ").x;
        if(bStroke)
            DrawString(insX, insY, "S", olc::RED);
        
        insX += GetTextSize("S)troke ").x;
        if(bWire)
            DrawString(insX, insY, "W", olc::RED);
            
        return !GetKey(olc::ESCAPE).bPressed;
    }

private:
    
    void SaveFile(const std::string& file)
    {
        outfile.open(file);
        
        for(auto& point : mModel)
        {
            olc::vf2d savedPoint = point * 0.1f;
            outfile << savedPoint.x << " " << savedPoint.y << std::endl;
        }
        outfile.close();
    }

private:
    
    // precalculated screen center
    olc::vi2d vCenter;
    
    // scaling
    float fScale;
    float fMinScale;
    float fMaxScale;

    // colors
    olc::Pixel cBackground;
    olc::Pixel cGrid;
    olc::Pixel cOrigin;

    // drawing states
    bool bFill;
    bool bPoints;
    bool bStroke;
    bool bWire;
    
    // collection of points
    Model mModel;

    // track selected point
    int selected;

    // file stream
    std::ofstream outfile;

};

int main()
{
    Modeler modeler;
    if(modeler.Construct(320, 180, 4, 4))
        modeler.Start();

    return 0;
}
