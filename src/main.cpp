#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

#define GFX_BLACKBOX_IMPLEMENTATION
#include "gfxBlackbox.h"

#if !defined(__EMSCRIPTEN__)
#include "portable-file-dialogs.h"
#endif

typedef std::vector<olc::vf2d> Model;

// #define HI_DEF

#if defined(HI_DEF)
#define POINT_MODIFIER  40.0f
#define SCALE_INIT      40.0f
#define SCALE_MIN       16.0f
#define SCALE_MAX       200.0f
#define STRING_SCALE    4
#else
#define POINT_MODIFIER  20.0f
#define SCALE_INIT      10.0f
#define SCALE_MIN       4.0f
#define SCALE_MAX       50.0f
#define STRING_SCALE    1
#endif

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
#if !defined(__EMSCRIPTEN__)
        // Check that a backend is available
        if (!pfd::settings::available())
        {
            std::cout << "Portable File Dialogs are not available on this platform.\n";
            return false;
        }
#endif        
        // canvas size
        vSize = { 120, 120 };
        vOrigin = vSize / 2.0f;

        // scaling
        fScale = SCALE_INIT;
        fMinScale = SCALE_MIN;
        fMaxScale = SCALE_MAX;

        vOffset = {
            -((vSize.x / 2 * fScale) - ScreenWidth() / 2),
            -((vSize.y / 2 * fScale) - ScreenHeight() / 2),
        };

        // colors
        cBackground = olc::VERY_DARK_BLUE;
        cCursor =     olc::DARK_CYAN;
        cGrid =       olc::DARK_BLUE;
        cGridWhole =  olc::VERY_DARK_RED;
        cOrigin =     olc::BLUE;

        // drawing states
        bFill =     true;
        bPoints =   true;
        bStroke =   true;
        bWire =     true;

        // point selection
        nSelected = -1;
        
        return true;
    }
    
    bool OnUserUpdate(float fElapsedTime) override
    {
        
        std::stringstream buf;
        vMouse = GetMousePos();
        nHover = -1;

        // BEGIN: pan
        if(GetMouse(2).bPressed)
            vPanStart = vMouse;
        
        if(GetMouse(2).bHeld)
        {
            vOffset += (vMouse - vPanStart);
            vPanStart = vMouse;
        }
        // END: pan
        
        // BEGIN: zoom
        vZoomStart = ScreenToWorld(vMouse);

        fScale += (float)GetMouseWheel() * 3.0f * fElapsedTime;
        
        // clamp zooming scale
        if(fScale < fMinScale) fScale = fMinScale;
        if(fScale > fMaxScale) fScale = fMaxScale;
        
        vOffset -= (vZoomStart - ScreenToWorld(vMouse)) * fScale;
        // END: zoom

        // find cursor location
        vCursor = olc::vf2d(
            round(vZoomStart.x - vOrigin.x),
            round(vZoomStart.y - vOrigin.y)
        );
        
        // prepare cursor for output
        buf << vCursor.x * 0.1f << " " << vCursor.y * 0.1f;
        
        // find if we're hovering over a point
        nHover = -1;
        for(int i = 0; i < mModel.size(); i++)
        {
            if((vZoomStart - mModel[i]).mag() < 0.5f)
            {
                nHover = i;
                break;
            }
        }
        
        // add or select point (Left Mouse)
        if(GetMouse(0).bPressed)
        {
            nSelected = -1;
            
            // are we hovering over one?
            if(nHover != -1)
                nSelected = nHover;

            // if we're here and not selected, add the point
            if(nSelected == -1)
                mModel.push_back(vCursor + vOrigin);
        }
        
        // move the selected point around while held
        if(GetMouse(0).bHeld && nSelected != -1)
            mModel[nSelected] = vCursor + vOrigin;

        // deselect point when released
        if(GetMouse(0).bReleased && nSelected != -1)
            nSelected = -1;

        // remove point we're hoving over
        if(GetKey(olc::R).bPressed && nHover != -1)
        {
            mModel.erase(mModel.begin()+nHover);
            nSelected = -1;
            nHover = -1;
        }
        
        // save model
        if(GetKey(olc::K1).bPressed)
        {
#if !defined(__EMSCRIPTEN__)
            auto dest = pfd::save_file("Select a file", ".model", {"Model Files", "*.model"}).result();
            if(!dest.empty())
                SaveFile(dest);
#else
            SaveFile("");
#endif
        }

        // controls
        if(GetKey(olc::F).bPressed) bFill = !bFill;
        if(GetKey(olc::P).bPressed) bPoints = !bPoints;
        if(GetKey(olc::S).bPressed) bStroke = !bStroke;
        if(GetKey(olc::W).bPressed) bWire = !bWire;
        
        // clear to black
        Clear(olc::BLACK);
        
        DrawCanvas();

        // get points transformed to screen space
        auto transformed = gfx_blackbox::Polygon::Transform(mModel, vOffset, fScale, 0.0f);
        
        if(bFill)
            gfx_blackbox::Polygon::Fill(this, transformed, olc::DARK_GREY);
        
        if(bWire)
            gfx_blackbox::Polygon::Wire(this, transformed, olc::GREY);
        
        if(bStroke)
            gfx_blackbox::Polygon::Stroke(this, transformed, olc::WHITE);

        DrawPoints(transformed);

        DrawString(olc::vi2d{5, 5} * STRING_SCALE, buf.str(), olc::WHITE, STRING_SCALE);
        DrawInstructions();

        return !GetKey(olc::ESCAPE).bPressed;
    }


private:

	olc::vf2d ScreenToWorld(const olc::vf2d& p)
	{
		return olc::vf2d(p - vOffset) / fScale;
	}

    void DrawCanvas()
    {
        // fill the canvas
        FillRect(vOffset, vSize * fScale, cBackground);
        
        // make it draw less grid when zoomed out
        int nGridIncrement = (fScale < 40.0f) ? 2 : 1;

        // draw columns
        for(int i = 0; i < vSize.x; i += nGridIncrement)
            DrawLine(std::floor((i * fScale) + vOffset.x), vOffset.y, std::floor((i * fScale) + vOffset.x), vSize.y * fScale + vOffset.y, cGrid);
        
        // draw rows
        for(int i = 0; i < vSize.y; i += nGridIncrement)
            DrawLine(vOffset.x, std::floor((i * fScale) + vOffset.y), vSize.x * fScale + vOffset.x, std::floor((i * fScale) + vOffset.y), cGrid);
        
        // draw whole numbered columns a different color than the rest
        for(int i = 0; i < vSize.x; i += 10)
            DrawLine(std::floor((i * fScale) + vOffset.x), vOffset.y, std::floor((i * fScale) + vOffset.x), vSize.y * fScale + vOffset.y, cGridWhole);

        // draw whole numbered rows a different color than the rest
        for(int i = 0; i < vSize.y; i += 10)
            DrawLine(vOffset.x, std::floor((i * fScale) + vOffset.y), vSize.x * fScale + vOffset.x, std::floor((i * fScale) + vOffset.y), cGridWhole);

        // draw origin
        DrawLine(std::floor((vSize.x / 2 * fScale) + vOffset.x), vOffset.y, std::floor((vSize.x / 2 * fScale) + vOffset.x), vSize.y * fScale + vOffset.y, cOrigin);
        DrawLine(vOffset.x, std::floor((vSize.y / 2 * fScale) + vOffset.y), vSize.x * fScale + vOffset.x, std::floor(((vSize.y / 2) * fScale) + vOffset.y), cOrigin);
    }

    void DrawPoints(const Model& model)
    {
        // do some math to derive the radius of our points
        float radius = (fScale / fMaxScale) * POINT_MODIFIER;
        
        // draw our mouse cursor
        FillCircle(((vCursor + vOrigin) * fScale) + vOffset, radius, cCursor);
        
        // draw points
        for(int i = 0; i < model.size(); i++)
        {
            if(!bPoints)
                break;

            // default red
            olc::Pixel cPoint = olc::RED;
            
            // yellow on hover
            if(nHover == i)
                cPoint = olc::YELLOW;
            
            // green on selected
            if(nSelected == i)
                cPoint = olc::GREEN;

            FillCircle(model[i], radius, cPoint);
            DrawStringDecal(model[i]+olc::vf2d{1,1}, std::to_string(i), olc::BLACK, olc::vf2d(radius, radius) * 0.3f);
            DrawStringDecal(model[i], std::to_string(i), olc::WHITE, olc::vf2d(radius, radius) * 0.3f);
        }
    }

    void DrawInstructions()
    {
        int insX = 5 * STRING_SCALE;
        int insY = ScreenHeight() - (14 * STRING_SCALE);

        // Instructions
        DrawString(insX, insY, "F)ill P)oints S)troke W)ire", olc::WHITE, STRING_SCALE);
        
        if(bFill)
            DrawString(insX, insY, "F", olc::RED, STRING_SCALE);
        
        insX += GetTextSize("F)ill ").x * STRING_SCALE;
        if(bPoints)
            DrawString(insX, insY, "P", olc::RED, STRING_SCALE);

        insX += GetTextSize("P)oints ").x * STRING_SCALE;
        if(bStroke)
            DrawString(insX, insY, "S", olc::RED, STRING_SCALE);
        
        insX += GetTextSize("S)troke ").x * STRING_SCALE;
        if(bWire)
            DrawString(insX, insY, "W", olc::RED, STRING_SCALE);
            
    }

    void SaveFile(const std::string& file)
    {
#if defined(__EMSCRIPTEN__)
        // cycle through the points
        for(auto& point : mModel)
        {
            // convert each point to the intended world space
            olc::vf2d savedPoint = (point - vOrigin) * 0.1f;
            
            // save the converted point to the file
            std::cout << savedPoint.x << " " << savedPoint.y << std::endl;
        }

#else
        // open the file
        outfile.open(file);
        
        // cycle through the points
        for(auto& point : mModel)
        {
            // convert each point to the intended world space
            olc::vf2d savedPoint = (point - vOrigin) * 0.1f;
            
            // save the converted point to the file
            outfile << savedPoint.x << " " << savedPoint.y << std::endl;
            
        }

        // close the file
        outfile.close();
#endif
    }

private:
    
    // mouse cursor
    olc::vf2d vCursor;

    // scaling
    float fScale;
    float fMinScale;
    float fMaxScale;

    // canvas size
    olc::vf2d vSize;
    olc::vf2d vOrigin;

    // pan and zoom 
    olc::vf2d vOffset;
    olc::vi2d vMouse;
    olc::vi2d vPanStart;
    olc::vf2d vZoomStart;

    // colors
    olc::Pixel cBackground;
    olc::Pixel cCursor;
    olc::Pixel cGrid;
    olc::Pixel cGridWhole;
    olc::Pixel cOrigin;

    // drawing states
    bool bFill;
    bool bPoints;
    bool bStroke;
    bool bWire;
    
    // collection of points
    Model mModel;

    // track selected point
    int nHover;
    int nSelected;

    // file stream
    std::ofstream outfile;
};

int main()
{
    Modeler modeler;
#if defined(HI_DEF)
    if(modeler.Construct(1280, 720, 1, 1))
#else
    if(modeler.Construct(320, 180, 4, 4))
#endif
        modeler.Start();

    return 0;
}
