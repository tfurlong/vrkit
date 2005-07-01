#ifndef IOV_UI_BUILDER_H_
#define IOV_UI_BUILDER_H_

#include <IOV/Config.h>
#include <OpenSG/OSGGeometry.h>
#include <OpenSG/OSGTextFace.h>
#include <OpenSG/OSGTextTXFFace.h>
#include <OpenSG/OSGTextTXFGlyph.h>
#include <OpenSG/OSGTextFaceFactory.h>
#include <OpenSG/OSGTextLayoutParam.h>
#include <OpenSG/OSGTextLayoutResult.h>

#include <gmtl/Math.h>

namespace inf
{

/** Helper class that implements builder pattern
 * for creating UI data in OpenSG.
 *
 * This class is designed to work with 2D data.
 * Assumes working in x,y plane with z out.
 */
class IOV_CLASS_API UiBuilder
{
public:

   /** Create a geom node core that can be used for building.
    * @note: All methods below will rely upon this method being used to create
    *        the geom core passed to them for building.
    */
   OSG::GeometryPtr createGeomGeo();

   /** Build a rectangle.
    * @PARAM geom    The geometry to add to.
    * @PARAM minPt   Minimum pt on the rectangle.
    * @PARAM maxPt   Maximum pt on the rectangle.
    * @PARAM alpha   Uniform alpha to apply to all vert colors.
    * @PARAM filled  If true the rectangle is a filled tri list, else it is a line loop.
    */
   void buildRectangleOutline(OSG::GeometryPtr geom, OSG::Color3f color, OSG::Pnt2f minPt, OSG::Pnt2f maxPt, float alpha=1.0);

   /** Build a 3D box.
    * @PARAM geom    The geometry to add to.
    * @PARAM minPt   Minimum pt on box.
    * @PARAM maxPt   Maximum pt on box.
    * @PARAM frontDepth  Z value of the front surface
    * @PARAM backDepth   Z value of the back surface.
    * @PARAM alpha   Uniform alpha to apply to all vert colors.
    * @note If front and back depth are equal, then we only draw front facing surface.
    */
   void buildRectangle(OSG::GeometryPtr geom, const OSG::Color3f color,
                       const OSG::Pnt2f minPt, const OSG::Pnt2f maxPt,
                       const float frontDepth=0, const float backDepth=0, const float alpha=1.0f);

   /** Build a 3D box with rounded corners.
    * @PARAM geom    The geometry to add to.
    * @PARAM minPt   Minimum pt on box.
    * @PARAM maxPt   Maximum pt on box.
    * @PARAM innerRad Inner radius.
    * @PARAM outerRad Outer radius.
    * @PARAM numSegs  Number of segments to use.
    * @PARAM frontDepth  Z value of the front surface
    * @PARAM backDepth   Z value of the back surface.
    * @PARAM alpha   Uniform alpha to apply to all vert colors.
    * @note If front and back depth are equal, then we only draw front facing surface.
    */
   void buildRoundedRectangle(OSG::GeometryPtr geom, const OSG::Color3f color,
                              const OSG::Pnt2f minPt, const OSG::Pnt2f maxPt,
                              const float innerRad, const float outerRad, const unsigned numSegs, const bool filled,
                              const float frontDepth=0, const float backDepth=0, const float alpha=1.0f);

   /** Build a 3D disc.
    * @PARAM geom    The geometry to add to.
    * @PARAM color   Color for the geometry
    * @PARAM center  Center point of the disc
    * @PARAM innerRad Inner radius.
    * @PARAM outerRad Outer radius.
    * @PARAM numSegs  Number of segments to use.
    * @PARAM startAngle  Starting angle in radians.
    * @PARAM endAngle    Ending angle in radians.
    * @PARAM frontDepth  Z value of the front surface
    * @PARAM backDepth   Z value of the back surface.
    * @note If front and back depth are equal, then we only draw front facing surface.
    */
   void buildDisc(OSG::GeometryPtr geom, const OSG::Color3f color, const OSG::Pnt2f center,
                  const float innerRad, const float outerRad, const unsigned numSegs, const float startAngle=0, const float endAngle=gmtl::Math::TWO_PI,
                  const float frontDepth=0.5, const float backDepth=-0.5, const bool capIt=true, const float alpha=1.0f);


   /** Helper class to use as a parameter to the text building methods. */
   class Font
   {
   public:
      Font(std::string family, OSG::TextFace::Style style = OSG::TextFace::STYLE_PLAIN, unsigned size = 48);

      void update();

   public:
      std::string          mFamilyName;
      OSG::TextFace::Style mStyle;
      OSG::TextTXFParam    mParams;    /**< Parameters used to generate the font (and update it). */
      OSG::TextTXFFace*    mFace;      /**< The face that we are using for this font. */
   };

   /** Create geometry to text management.
    * @post New geom is returned.  Has ChunkMaterial for mat.
    */
   OSG::GeometryPtr createTextGeom();

   /** Replace the geomtetry in the area here.
    * @note The font passed is used to update the geom texture for the glyphs.
    */
   void buildText(OSG::GeometryPtr geom, UiBuilder::Font& font, std::string text, OSG::Vec2f offset=OSG::Vec2f(0,0),
                  OSG::Color3f color=OSG::Color3f(1,1,1), float scale=1.0f, float spacing=1.0f);

   /** Add text to the geometry.
    * @note The font is NOT used to set the texture.  It must be the same as what you used to build the text geom.
    */
   void addText(OSG::GeometryPtr geom, UiBuilder::Font& font, std::string text, OSG::Vec2f offset=OSG::Vec2f(0,0),
                  OSG::Color3f color=OSG::Color3f(1,1,1), float scale=1.0f, float spacing=1.0f);


   /** Get the size that the text will take up on screen. */
   OSG::Vec2f getTextSize(UiBuilder::Font& font, std::string text, float spacing=1.0f);

};

}  // namespace IOV
#endif
