#include "gfx_mesh.h"
#include "vegastrike.h"
#include <iostream>
#include <fstream>
#include <expat.h>
#include <float.h>
#include <assert.h>
#ifdef WIN32
#else
#include <values.h>
#endif
#include "xml_support.h"
#include "gfx_transform_vector.h"
#ifdef max
#undef max
#endif

static inline float max(float x, float y) {
  if(x>y) return x;
  else return y;
}

#ifdef min
#undef min
#endif

static inline float min(float x, float y) {
  if(x<y) return x;
  else return y;
}

float scale=1;

using XMLSupport::EnumMap;
using XMLSupport::Attribute;
using XMLSupport::AttributeList;
using XMLSupport::parse_float;
using XMLSupport::parse_int;
struct GFXMaterial;
const EnumMap::Pair Mesh::XML::element_names[] = {
  EnumMap::Pair("UNKNOWN", XML::UNKNOWN),
  EnumMap::Pair("Material", XML::MATERIAL),
  EnumMap::Pair("Ambient", XML::AMBIENT),
  EnumMap::Pair("Diffuse", XML::DIFFUSE),
  EnumMap::Pair("Specular", XML::SPECULAR),
  EnumMap::Pair("Emissive", XML::EMISSIVE),
  EnumMap::Pair("Mesh", XML::MESH),
  EnumMap::Pair("Points", XML::POINTS),
  EnumMap::Pair("Point", XML::POINT),
  EnumMap::Pair("Location", XML::LOCATION),
  EnumMap::Pair("Normal", XML::NORMAL),
  EnumMap::Pair("Polygons", XML::POLYGONS),
  EnumMap::Pair("Line", XML::LINE),
  EnumMap::Pair("Tri", XML::TRI),
  EnumMap::Pair("Quad", XML::QUAD),
  EnumMap::Pair("Linestrip",XML::LINESTRIP),
  EnumMap::Pair("Tristrip", XML::TRISTRIP),
  EnumMap::Pair("Trifan", XML::TRIFAN),
  EnumMap::Pair("Quadstrip", XML::QUADSTRIP),
  EnumMap::Pair("Vertex", XML::VERTEX),
  EnumMap::Pair("Logo", XML::LOGO),
  EnumMap::Pair("Ref",XML::REF)
};

const EnumMap::Pair Mesh::XML::attribute_names[] = {
  EnumMap::Pair("UNKNOWN", XML::UNKNOWN),
  EnumMap::Pair("Scale",XML::SCALE),
  EnumMap::Pair("Blend",XML::BLENDMODE),
  EnumMap::Pair("texture", XML::TEXTURE),
  EnumMap::Pair("alphamap", XML::ALPHAMAP),
  EnumMap::Pair("red", XML::RED),
  EnumMap::Pair("green", XML::GREEN),
  EnumMap::Pair("blue", XML::BLUE),
  EnumMap::Pair("alpha", XML::ALPHA),
  EnumMap::Pair("power", XML::POWER),
  EnumMap::Pair("reflect", XML::REFLECT),
  EnumMap::Pair("x", XML::X),
  EnumMap::Pair("y", XML::Y),
  EnumMap::Pair("z", XML::Z),
  EnumMap::Pair("i", XML::I),
  EnumMap::Pair("j", XML::J),
  EnumMap::Pair("k", XML::K),
  EnumMap::Pair("Shade", XML::FLATSHADE),
  EnumMap::Pair("point", XML::POINT),
  EnumMap::Pair("s", XML::S),
  EnumMap::Pair("t", XML::T),
  //Logo stuffs
  EnumMap::Pair("Type",XML::TYPE),
  EnumMap::Pair("Rotate", XML::ROTATE),
  EnumMap::Pair("Weight", XML::WEIGHT),
  EnumMap::Pair("Size", XML::SIZE),
  EnumMap::Pair("Offset",XML::OFFSET),
};

const EnumMap Mesh::XML::element_map(XML::element_names, 22);
const EnumMap Mesh::XML::attribute_map(XML::attribute_names, 26);

void Mesh::beginElement(void *userData, const XML_Char *name, const XML_Char **atts) {
  ((Mesh*)userData)->beginElement(name, AttributeList(atts));
}

void Mesh::endElement(void *userData, const XML_Char *name) {
  ((Mesh*)userData)->endElement(name);
}

enum BLENDFUNC parse_alpha (char * tmp ) {
  if (strcmp (tmp,"ZERO")==0) {
    return ZERO;
  }
  if (strcmp (tmp,"ONE")==0) {
    return ONE;
  }
  if (strcmp (tmp,"SRCCOLOR")==0) {
    return SRCCOLOR;
  }
  if (strcmp (tmp,"INVSRCCOLOR")==0) {
    return INVSRCCOLOR;
  }
  if (strcmp (tmp,"SRCALPHA")==0) {
    return SRCALPHA;
  }
  if (strcmp (tmp,"INVSRCALPHA")==0) {
    return INVSRCALPHA;
  }
  if (strcmp (tmp,"DESTALPHA")==0) {
    return DESTALPHA;
  }
  if (strcmp (tmp,"INVDESTALPHA")==0) {
    return INVDESTALPHA;
  }
  if (strcmp (tmp,"DESTCOLOR")==0) {
    return DESTCOLOR;
  }
  if (strcmp (tmp,"INVDESTCOLOR")==0) {
    return INVDESTCOLOR;
  }
  if (strcmp (tmp,"SRCALPHASAT")==0) {
    return SRCALPHASAT;
  }
  if (strcmp (tmp,"CONSTALPHA")==0) {
    return CONSTALPHA;
  }
  if (strcmp (tmp,"INVCONSTALPHA")==0) {
    return INVCONSTALPHA;
  }
  if (strcmp (tmp,"CONSTCOLOR")==0) {
    return CONSTCOLOR;
  }
  if (strcmp (tmp,"INVCONSTCOLOR")==0) {
    return INVCONSTCOLOR;
  }
  return ZERO;
}


/* Load stages:
0 - no tags seen
1 - waiting for points
2 - processing points 
3 - done processing points, waiting for face data

Note that this is only a debugging aid. Once DTD is written, there
will be no need for this sort of checking
 */

void Mesh::beginElement(const string &name, const AttributeList &attributes) {
  //cerr << "Start tag: " << name << endl;
  //static bool flatshadeit=false;
  AttributeList::const_iterator iter;

  XML::Names elem = (XML::Names)XML::element_map.lookup(name);
  XML::Names top;
  if(xml->state_stack.size()>0) top = *xml->state_stack.rbegin();
  xml->state_stack.push_back(elem);

  bool texture_found = false;
  bool alpha_found = false;
  switch(elem) {
	  case XML::MATERIAL:
		  assert(xml->load_stage==4);
		  xml->load_stage=7;
		  for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
		    switch(XML::attribute_map.lookup((*iter).name)) {
		    case XML::POWER:
		      xml->material.power=parse_float((*iter).value);
		      break;
		    case XML::REFLECT:
		      if (strcmp((*iter).value.c_str(),"0")==0||strcmp((*iter).value.c_str(),"FALSE")==0)
			envMap=GFXFALSE;
		      break;
		    }

		  }
		  break;
  case XML::DIFFUSE:
	  assert(xml->load_stage==7);
	  xml->load_stage=8;
	  for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
		  switch(XML::attribute_map.lookup((*iter).name)) {
		  case XML::RED:
			  xml->material.dr=parse_float((*iter).value);
			  break;
		  case XML::BLUE:
			  xml->material.db=parse_float((*iter).value);
			  break;
		  case XML::ALPHA:
			  xml->material.da=parse_float((*iter).value);
			  break;
		  case XML::GREEN:
			  xml->material.dg=parse_float((*iter).value);
			  break;
		  }
	  }
	  break;
  case XML::EMISSIVE:
	  assert(xml->load_stage==7);
	  xml->load_stage=8;
	  for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
		  switch(XML::attribute_map.lookup((*iter).name)) {
		  case XML::RED:
			  xml->material.er=parse_float((*iter).value);
			  break;
		  case XML::BLUE:
			  xml->material.eb=parse_float((*iter).value);
			  break;
		  case XML::ALPHA:
			  xml->material.ea=parse_float((*iter).value);
			  break;
		  case XML::GREEN:
			  xml->material.eg=parse_float((*iter).value);
			  break;
		  }
	  }
	  break;
  case XML::SPECULAR:
	  assert(xml->load_stage==7);
	  xml->load_stage=8;
	  for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
		  switch(XML::attribute_map.lookup((*iter).name)) {
		  case XML::RED:
			  xml->material.sr=parse_float((*iter).value);
			  break;
		  case XML::BLUE:
			  xml->material.sb=parse_float((*iter).value);
			  break;
		  case XML::ALPHA:
			  xml->material.sa=parse_float((*iter).value);
			  break;
		  case XML::GREEN:
			  xml->material.sg=parse_float((*iter).value);
			  break;
		  }
	  }
	  break;
  case XML::AMBIENT:
	  assert(xml->load_stage==7);
	  xml->load_stage=8;
	  for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
		  switch(XML::attribute_map.lookup((*iter).name)) {
		  case XML::RED:
			  xml->material.ar=parse_float((*iter).value);
			  break;
		  case XML::BLUE:
			  xml->material.ab=parse_float((*iter).value);
			  break;
		  case XML::ALPHA:
			  xml->material.aa=parse_float((*iter).value);
			  break;
		  case XML::GREEN:
			  xml->material.ag=parse_float((*iter).value);
			  break;
		  }
	  }
	  break;
  case XML::UNKNOWN:
   fprintf (stderr, "Unknown element start tag '%s' detected\n",name.c_str());
    break;
  case XML::MESH:
    assert(xml->load_stage == 0);
    assert(xml->state_stack.size()==1);

    xml->load_stage = 1;
    // Read in texture attribute
    xml->decal_name = string("\0");
    xml->alpha_name = string("\0");
    char csrc[128];
    char cdst[128];
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(XML::attribute_map.lookup((*iter).name)) {
      case XML::TEXTURE:
	xml->decal_name = (*iter).value;
	texture_found = true;
	break;
      case XML::ALPHAMAP:
	xml->alpha_name = (*iter).value;
	alpha_found = true;
	break;
      case XML::SCALE:
	scale =  parse_float ((*iter).value);
	break;
      case XML::BLENDMODE:
	sscanf (((*iter).value).c_str(),"%s %s",csrc,cdst);
	blendSrc = parse_alpha (csrc);
	blendDst = parse_alpha (cdst);
	if (blendDst==blendSrc&&blendSrc==ZERO) {
	  blendSrc=ONE;
	}
	break;
      }

    }
    assert(texture_found);
    break;
  case XML::POINTS:
    assert(top==XML::MESH);
    assert(xml->load_stage == 1);

    xml->load_stage = 2;
    break;
  case XML::POINT:
    assert(top==XML::POINTS);
    
    memset(&xml->vertex, 0, sizeof(xml->vertex));
    xml->point_state = 0; // Point state is used to check that all necessary attributes are recorded
    break;
  case XML::LOCATION:
    assert(top==XML::POINT);

    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(XML::attribute_map.lookup((*iter).name)) {
      case XML::UNKNOWN:
	fprintf (stderr, "Unknown attribute '%s' encountered in Location tag\n",(*iter).name.c_str());
	break;
      case XML::X:
	assert(!(xml->point_state & XML::P_X));
	xml->vertex.x = parse_float((*iter).value);
	xml->vertex.i = 0;
	xml->point_state |= XML::P_X;
	break;
      case XML::Y:
	assert(!(xml->point_state & XML::P_Y));
	xml->vertex.y = parse_float((*iter).value);
	xml->vertex.j = 0;
	xml->point_state |= XML::P_Y;
	break;
      case XML::Z:
	assert(!(xml->point_state & XML::P_Z));
	xml->vertex.z = parse_float((*iter).value);
	xml->vertex.k = 0;
	xml->point_state |= XML::P_Z;
	break;
      default:
	assert(0);
      }
    }
    assert(xml->point_state & (XML::P_X |
			       XML::P_Y |
			       XML::P_Z) == 
	   (XML::P_X |
	    XML::P_Y |
	    XML::P_Z) );
    break;
  case XML::NORMAL:
    assert(top==XML::POINT);

    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(XML::attribute_map.lookup((*iter).name)) {
      case XML::UNKNOWN:
	fprintf (stderr, "Unknown attribute '%s' encountered in Normal tag\n",(*iter).name.c_str());
	break;
      case XML::I:
	assert(!(xml->point_state & XML::P_I));
	xml->vertex.i = parse_float((*iter).value);
	xml->point_state |= XML::P_I;
	break;
      case XML::J:
	assert(!(xml->point_state & XML::P_J));
	xml->vertex.j = parse_float((*iter).value);
	xml->point_state |= XML::P_J;
	break;
      case XML::K:
	assert(!(xml->point_state & XML::P_K));
	xml->vertex.k = parse_float((*iter).value);
	xml->point_state |= XML::P_K;
	break;
      default:
	assert(0);
      }
    }
    if (xml->point_state & (XML::P_I |
			       XML::P_J |
			       XML::P_K) != 
	   (XML::P_I |
	    XML::P_J |
	    XML::P_K) ) {
      if (!xml->recalc_norm) {
	fprintf(stderr,"Invalid Normal Data for point: <%f,%f,%f>\n",xml->vertex.x,xml->vertex.y, xml->vertex.z);
	xml->vertex.i=xml->vertex.j=xml->vertex.k=0;
	xml->recalc_norm=true;
      }
    }
    break;
  case XML::POLYGONS:
    assert(top==XML::MESH);
    assert(xml->load_stage==3);

    xml->load_stage = 4;
    break;

  case XML::LINE:
    assert(top==XML::POLYGONS);
    assert(xml->load_stage==4);
    xml->num_vertices=2;
    xml->active_list = &xml->lines;
    xml->active_ind = &xml->lineind;
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(XML::attribute_map.lookup((*iter).name)) {
      case XML::UNKNOWN:
	fprintf (stderr,"Unknown attribute '%s' encountered in Vertex tag\n",(*iter).name.c_str() );
	break;
      case XML::FLATSHADE:
	if ((*iter).value=="Flat") {
	  fprintf (stderr,"Cannot Flatshade Lines\n");
	}else {
	  if ((*iter).value=="Smooth") {
	    //ignored -- already done
	  }
	}
	break;
      default:
	assert (0);
      }
    }
    break;

  case XML::TRI:
    assert(top==XML::POLYGONS);
    assert(xml->load_stage==4);
    xml->num_vertices=3;
    xml->active_list = &xml->tris;
    xml->active_ind = &xml->triind;
    xml->trishade.push_back (0);
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(XML::attribute_map.lookup((*iter).name)) {
      case XML::UNKNOWN:
	fprintf (stderr,"Unknown attribute '%s' encountered in Vertex tag\n",(*iter).name.c_str() );
	break;
      case XML::FLATSHADE:
	if ((*iter).value=="Flat") {
	  xml->trishade[xml->trishade.size()-1]=1;
	}else {
	  if ((*iter).value=="Smooth") {
	    xml->trishade[xml->trishade.size()-1]=0;
	  }
	}
	break;
      default:
	assert (0);
      }
    }
    break;

  case XML::LINESTRIP:
    assert(top==XML::POLYGONS);
    assert(xml->load_stage==4);
    xml->num_vertices=2;
    xml->linestrips.push_back (vector<GFXVertex>());
    xml->active_list = &(xml->linestrips[xml->linestrips.size()-1]);
    xml->lstrcnt = xml->linestripind.size();
    xml->active_ind = &xml->linestripind;
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(XML::attribute_map.lookup((*iter).name)) {
      case XML::UNKNOWN:
	fprintf (stderr,"Unknown attribute '%s' encountered in Vertex tag\n",(*iter).name.c_str() );
	break;
      case XML::FLATSHADE:
	if ((*iter).value=="Flat") {
	  fprintf(stderr,"Cannot Flatshade Linestrips\n");
	}else {
	  if ((*iter).value=="Smooth") {
	    //ignored -- already done
	  }
	}
	break;
      default:
	assert (0);
      }
    }
    break;

  case XML::TRISTRIP:
    assert(top==XML::POLYGONS);
    assert(xml->load_stage==4);
    xml->num_vertices=3;//minimum number vertices
    xml->tristrips.push_back (vector<GFXVertex>());
    xml->active_list = &(xml->tristrips[xml->tristrips.size()-1]);
    xml->tstrcnt = xml->tristripind.size();
    xml->active_ind = &xml->tristripind;
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(XML::attribute_map.lookup((*iter).name)) {
      case XML::UNKNOWN:
	fprintf (stderr,"Unknown attribute '%s' encountered in Vertex tag\n",(*iter).name.c_str() );
	break;
      case XML::FLATSHADE:
	if ((*iter).value=="Flat") {
	  fprintf(stderr,"Cannot Flatshade Tristrips\n");
	}else {
	  if ((*iter).value=="Smooth") {
	    //ignored -- already done
	  }
	}
	break;
      default:
	assert (0);
      }
    }
    break;

  case XML::TRIFAN:
    assert(top==XML::POLYGONS);
    assert(xml->load_stage==4);
    xml->num_vertices=3;//minimum number vertices
    xml->trifans.push_back (vector<GFXVertex>());
    xml->active_list = &(xml->trifans[xml->trifans.size()-1]);
    xml->tfancnt = xml->trifanind.size();
    xml->active_ind = &xml->trifanind;
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(XML::attribute_map.lookup((*iter).name)) {
      case XML::UNKNOWN:
	fprintf (stderr,"Unknown attribute '%s' encountered in Vertex tag\n",(*iter).name.c_str() );
	break;
      case XML::FLATSHADE:
	if ((*iter).value=="Flat") {
	  fprintf (stderr,"Cannot Flatshade Trifans\n");
	}else {
	  if ((*iter).value=="Smooth") {
	    //ignored -- already done
	  }
	}
	break;
      default:
	assert (0);
      }
    }
    break;

  case XML::QUADSTRIP:
    assert(top==XML::POLYGONS);
    assert(xml->load_stage==4);
    xml->num_vertices=4;//minimum number vertices
    xml->quadstrips.push_back (vector<GFXVertex>());
    xml->active_list = &(xml->quadstrips[xml->quadstrips.size()-1]);
    xml->qstrcnt = xml->quadstripind.size();
    xml->active_ind = &xml->quadstripind;
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(XML::attribute_map.lookup((*iter).name)) {
      case XML::UNKNOWN:
	fprintf (stderr,"Unknown attribute '%s' encountered in Vertex tag\n",(*iter).name.c_str() );
	break;
      case XML::FLATSHADE:
	if ((*iter).value=="Flat") {
	  fprintf (stderr, "Cannot Flatshade Quadstrips\n");
	}else {
	  if ((*iter).value=="Smooth") {
	    //ignored -- already done
	  }
	}
	break;
      default:
	assert (0);
      }
    }
    break;
   
  case XML::QUAD:
    assert(top==XML::POLYGONS);
    assert(xml->load_stage==4);
    xml->num_vertices=4;
    xml->active_list = &xml->quads;
    xml->active_ind = &xml->quadind;
    xml->quadshade.push_back (0);
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(XML::attribute_map.lookup((*iter).name)) {
      case XML::UNKNOWN:
	fprintf (stderr,"Unknown attribute '%s' encountered in Vertex tag\n",(*iter).name.c_str() );
	break;
      case XML::FLATSHADE:
	if ((*iter).value=="Flat") {
	  xml->quadshade[xml->quadshade.size()-1]=1;
	}else {
	  if ((*iter).value=="Smooth") {
	    xml->quadshade[xml->quadshade.size()-1]=0;
	  }
	}
	break;
      default:
	assert(0);
      }
    }
    break;
  case XML::VERTEX:
    assert(top==XML::TRI || top==XML::QUAD || top==XML::LINE ||top ==XML::TRISTRIP || top ==XML::TRIFAN||top ==XML::QUADSTRIP || top==XML::LINESTRIP);
    assert(xml->load_stage==4);

    xml->vertex_state = 0;
    unsigned int index;
    float s, t;
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(XML::attribute_map.lookup((*iter).name)) {
      case XML::UNKNOWN:
	fprintf (stderr,"Unknown attribute '%s' encountered in Vertex tag\n",(*iter).name.c_str() );
	break;
      case XML::POINT:
	assert(!(xml->vertex_state & XML::V_POINT));
	xml->vertex_state |= XML::V_POINT;
	index = parse_int((*iter).value);
	break;
      case XML::S:
	assert(!(xml->vertex_state & XML::V_S));
	xml->vertex_state |= XML::V_S;
	s = parse_float((*iter).value);
	break;
      case XML::T:
	assert(!(xml->vertex_state & XML::V_T));
	xml->vertex_state |= XML::V_T;
	t = parse_float((*iter).value);
	break;
      default:
	assert(0);
     }
    }
    assert(xml->vertex_state & (XML::V_POINT|
				XML::V_S|
				XML::V_T) == 
	   (XML::V_POINT|
	    XML::V_S|
	    XML::V_T) );
    assert(index < xml->vertices.size());

    memset(&xml->vertex, 0, sizeof(xml->vertex));
    xml->vertex = xml->vertices[index];
    xml->vertexcount[index]+=1;
    if ((!xml->vertex.i)&&(!xml->vertex.j)&&(!xml->vertex.k)) {
      if (!xml->recalc_norm) {
	fprintf(stderr,"Invalid Normal Data for point: <%f,%f,%f>\n",xml->vertex.x,xml->vertex.y, xml->vertex.z);
	
	xml->recalc_norm=true;
      }
    }
    //        xml->vertex.x*=scale;
    //        xml->vertex.y*=scale;
    //        xml->vertex.z*=scale;//FIXME
    xml->vertex.s = s;
    xml->vertex.t = t;
    xml->active_list->push_back(xml->vertex);
    xml->active_ind->push_back(index);
    xml->num_vertices--;
    break;
  case XML::LOGO: 
    assert (top==XML::MESH);
    assert (xml->load_stage==4);
    xml->load_stage=5;
    xml->vertex_state=0;
    unsigned int typ;
    float rot, siz,offset;
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(XML::attribute_map.lookup((*iter).name)) {
      case XML::UNKNOWN:
	fprintf (stderr,"Unknown attribute '%s' encountered in Vertex tag\n",(*iter).name.c_str() );
	break;
      case XML::TYPE:
	assert (!(xml->vertex_state&XML::V_TYPE));
	xml->vertex_state|=XML::V_TYPE;
	typ = parse_int((*iter).value);
	
	break;
      case XML::ROTATE:
	assert (!(xml->vertex_state&XML::V_ROTATE));
	xml->vertex_state|=XML::V_ROTATE;
	rot = parse_float((*iter).value);

	break;
      case XML::SIZE:
	assert (!(xml->vertex_state&XML::V_SIZE));
	xml->vertex_state|=XML::V_SIZE;
	siz = parse_float((*iter).value);
	break;
      case XML::OFFSET:
	assert (!(xml->vertex_state&XML::V_OFFSET));
	xml->vertex_state|=XML::V_OFFSET;
	offset = parse_float ((*iter).value);
	break;
      default:
	assert(0);
     }
    }

    assert(xml->vertex_state & (XML::V_TYPE|
				XML::V_ROTATE|
				XML::V_SIZE|
				XML::V_OFFSET) == 
	   (XML::V_TYPE|
	    XML::V_ROTATE|
	    XML::V_SIZE|
	    XML::V_OFFSET) );
    xml->logos.push_back(XML::ZeLogo());
    xml->logos[xml->logos.size()-1].type = typ;
    xml->logos[xml->logos.size()-1].rotate = rot;
    xml->logos[xml->logos.size()-1].size = siz;
    xml->logos[xml->logos.size()-1].offset = offset;
    break;
  case XML::REF:
    assert (top==XML::LOGO);
    assert (xml->load_stage==5);
    xml->load_stage=6;
    unsigned int ind;
    float indweight;
    int ttttttt;
    ttttttt=0;
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(XML::attribute_map.lookup((*iter).name)) {
      case XML::UNKNOWN:
	fprintf (stderr,"Unknown attribute '%s' encountered in Vertex tag\n",(*iter).name.c_str() );
	break;
      case XML::POINT:
	assert (ttttttt<2);
	xml->vertex_state |= XML::V_POINT;
	ind = parse_int((*iter).value);
	ttttttt+=2;
	break;
      case XML::WEIGHT:
	assert ((ttttttt&1)==0);
	ttttttt+=1;
	xml->vertex_state |= XML::V_S;
	indweight = parse_float((*iter).value);
	break;
      default:
	assert(0);
     }
    }
    assert (ttttttt==3);
    xml->logos[xml->logos.size()-1].refpnt.push_back(ind);
    xml->logos[xml->logos.size()-1].refweight.push_back(indweight);
    xml->vertex_state+=XML::V_REF;
    break;
  default:
    assert(0);
  }
}

void Mesh::endElement(const string &name) {
  //cerr << "End tag: " << name << endl;

  XML::Names elem = (XML::Names)XML::element_map.lookup(name);
  assert(*xml->state_stack.rbegin() == elem);
  xml->state_stack.pop_back();
  unsigned int i;
  switch(elem) {
  case XML::UNKNOWN:
    fprintf (stderr,"Unknown element end tag '%s' detected\n",name.c_str());
    break;
  case XML::POINT:
    assert(xml->point_state & (XML::P_X | 
			       XML::P_Y | 
			       XML::P_Z |
			       XML::P_I |
			       XML::P_J |
			       XML::P_K) == 
	   (XML::P_X | 
	    XML::P_Y | 
	    XML::P_Z |
	    XML::P_I |
	    XML::P_J |
	    XML::P_K) );
    xml->vertices.push_back(xml->vertex);
    xml->vertexcount.push_back(0);
    break;
  case XML::POINTS:
    xml->load_stage = 3;

    /*
    cerr << xml->vertices.size() << " vertices\n";
    for(int a=0; a<xml->vertices.size(); a++) {
      clog << "Point: (" << xml->vertices[a].x << ", " << xml->vertices[a].y << ", " << xml->vertices[a].z << ") (" << xml->vertices[a].i << ", " << xml->vertices[a].j << ", " << xml->vertices[a].k << ")\n";
    }
    clog << endl;
    */
    break;
  case XML::LINE:
    assert (xml->num_vertices==0);
    break;
  case XML::TRI:
    assert(xml->num_vertices==0);
    break;
  case XML::QUAD:
    assert(xml->num_vertices==0);
    break;
  case XML::LINESTRIP:
    assert (xml->num_vertices<=0);
    for (i=xml->lstrcnt+1;i<xml->linestripind.size();i++) {
      xml->nrmllinstrip.push_back (xml->linestripind[i-1]);
      xml->nrmllinstrip.push_back (xml->linestripind[i]);
    }
    break;
  case XML::TRISTRIP:
    assert(xml->num_vertices<=0);   
    for (i=xml->tstrcnt+2;i<xml->tristripind.size();i++) {
      if ((i-xml->tstrcnt)%2) {
	//normal order
	xml->nrmltristrip.push_back (xml->tristripind[i-2]);
	xml->nrmltristrip.push_back (xml->tristripind[i-1]);
	xml->nrmltristrip.push_back (xml->tristripind[i]);
      } else {
	//reverse order
	xml->nrmltristrip.push_back (xml->tristripind[i-1]);
	xml->nrmltristrip.push_back (xml->tristripind[i-2]);
	xml->nrmltristrip.push_back (xml->tristripind[i]);
      }
    }
    break;
  case XML::TRIFAN:
    assert (xml->num_vertices<=0);
    for (i=xml->tfancnt+2;i<xml->trifanind.size();i++) {
      xml->nrmltrifan.push_back (xml->trifanind[xml->tfancnt]);
      xml->nrmltrifan.push_back (xml->trifanind[i-1]);
      xml->nrmltrifan.push_back (xml->trifanind[i]);
    }
    break;
  case XML::QUADSTRIP://have to fix up nrmlquadstrip so that it 'looks' like a quad list for smooth shading
    assert(xml->num_vertices<=0);
    for (i=xml->qstrcnt+3;i<xml->quadstripind.size();i+=2) {
      xml->nrmlquadstrip.push_back (xml->quadstripind[i-3]);
      xml->nrmlquadstrip.push_back (xml->quadstripind[i-2]);
      xml->nrmlquadstrip.push_back (xml->quadstripind[i]);
      xml->nrmlquadstrip.push_back (xml->quadstripind[i-1]);
    }
    break;
  case XML::POLYGONS:
    assert(xml->tris.size()%3==0);
    assert(xml->quads.size()%4==0);
    break;
  case XML::REF:
    assert (xml->load_stage==6);
    xml->load_stage=5;
    break;
  case XML::LOGO:
    assert (xml->load_stage==5);
    assert (xml->vertex_state>=XML::V_REF*3);//make sure there are at least 3 reference points
    xml->load_stage=4;
    break;
  case XML::MATERIAL:
	  assert(xml->load_stage==7);
	  xml->load_stage=4;
	  break;
  case XML::DIFFUSE:
	  assert(xml->load_stage==8);
	  xml->load_stage=7;
	  break;
  case XML::EMISSIVE:
	  assert(xml->load_stage==8);
	  xml->load_stage=7;
	  break;
  case XML::SPECULAR:
	  assert(xml->load_stage==8);
	  xml->load_stage=7;
	  break;
  case XML::AMBIENT:
	  assert(xml->load_stage==8);
	  xml->load_stage=7;
	  break;
  case XML::MESH:
    assert(xml->load_stage==4);//4 is done with poly, 5 is done with Logos

    xml->load_stage=5;
    break;
  default:
    ;
  }
}



void SumNormals (int trimax, int t3vert, 
		 vector <GFXVertex> &vertices,
		 vector <int> &triind,
		 vector <int> &vertexcount,
		 bool * vertrw ) {
  int a=0;
  int i=0;
  int j=0;
  if (t3vert==2) {//oh man we have a line--what to do, what to do!
    for (i=0;i<trimax;i++,a+=t3vert) {
      Vector Cur (vertices[triind[a]].x-vertices[triind[a+1]].x,
		  vertices[triind[a]].y-vertices[triind[a+1]].y,
		  vertices[triind[a]].z-vertices[triind[a+1]].z);
      Normalize (Cur);
      vertices[triind[a+1]].i += Cur.i/vertexcount[triind[a+1]];
      vertices[triind[a+1]].j += Cur.j/vertexcount[triind[a+1]];
      vertices[triind[a+1]].k += Cur.k/vertexcount[triind[a+1]];

      vertices[triind[a]].i -= Cur.i/vertexcount[triind[a]];
      vertices[triind[a]].j -= Cur.j/vertexcount[triind[a]];
      vertices[triind[a]].k -= Cur.k/vertexcount[triind[a]];
    }
    return; //bye bye mrs american pie 
  }
  for (i=0;i<trimax;i++,a+=t3vert) {
    for (j=0;j<t3vert;j++) {
      if (vertrw[triind[a+j]]) {
	Vector Cur (vertices[triind[a+j]].x,
		    vertices[triind[a+j]].y,
		    vertices[triind[a+j]].z);
	Cur = (Vector (vertices[triind[a+((j+2)%t3vert)]].x,
		       vertices[triind[a+((j+2)%t3vert)]].y,
		       vertices[triind[a+((j+2)%t3vert)]].z)-Cur)
	  .Cross(Vector (vertices[triind[a+((j+1)%t3vert)]].x,
			 vertices[triind[a+((j+1)%t3vert)]].y,
			 vertices[triind[a+((j+1)%t3vert)]].z)-Cur);
	Normalize(Cur);
	//Cur = Cur*(1.00F/xml->vertexcount[a+j]);
	vertices[triind[a+j]].i+=Cur.i/vertexcount[triind[a+j]];
	vertices[triind[a+j]].j+=Cur.j/vertexcount[triind[a+j]];
	vertices[triind[a+j]].k+=Cur.k/vertexcount[triind[a+j]];
      }
    }
  }
}

void updateMax (float &minSizeX, float &maxSizeX, float &minSizeY, float &maxSizeY, float &minSizeZ, float &maxSizeZ, const GFXVertex &ver) {
    minSizeX = min(ver.x, minSizeX);
    maxSizeX = max(ver.x, maxSizeX);
    minSizeY = min(ver.y, minSizeY);
    maxSizeY = max(ver.y, maxSizeY);
    minSizeZ = min(ver.z, minSizeZ);
    maxSizeZ = max(ver.z, maxSizeZ);
}
void Mesh::LoadXML(const char *filename, Mesh *oldmesh) {
  const int chunk_size = 16384;
  
  FILE* inFile = fopen (filename, "r+b");
  if(!inFile) {
    assert(0);
    return;
  }

  xml = new XML;
  xml->load_stage = 0;
  xml->recalc_norm=false;
  XML_Parser parser = XML_ParserCreate(NULL);
  XML_SetUserData(parser, this);
  XML_SetElementHandler(parser, &Mesh::beginElement, &Mesh::endElement);
  
  do {
    char *buf = (XML_Char*)XML_GetBuffer(parser, chunk_size);
    int length;
    
    length = fread(buf,1, chunk_size,inFile);
    //length = inFile.gcount();
    XML_ParseBuffer(parser, length, feof(inFile));
  } while(!feof(inFile));
	fclose (inFile);
  // Now, copy everything into the mesh data structures
  assert(xml->load_stage==5);
  //begin vertex normal calculations if necessary
  if (xml->recalc_norm) {
    unsigned int i; unsigned int a=0;
    unsigned int j;

    bool *vertrw = new bool [xml->vertices.size()]; 
    for (i=0;i<xml->vertices.size();i++) {
      if (xml->vertices[i].i==0&&
	  xml->vertices[i].j==0&&
	  xml->vertices[i].k==0) {
	vertrw[i]=true;
      }else {
	vertrw[i]=false;
      }
    }
    SumNormals (xml->tris.size()/3,3,xml->vertices, xml->triind,xml->vertexcount, vertrw);
    SumNormals (xml->quads.size()/4,4,xml->vertices, xml->quadind,xml->vertexcount, vertrw);
    SumNormals (xml->lines.size()/2,2,xml->vertices,xml->lineind,xml->vertexcount,vertrw);
    SumNormals (xml->nrmltristrip.size()/3,3,xml->vertices, xml->nrmltristrip,xml->vertexcount, vertrw);
    SumNormals (xml->nrmltrifan.size()/3,3,xml->vertices, xml->nrmltrifan,xml->vertexcount, vertrw);
    SumNormals (xml->nrmlquadstrip.size()/4,4,xml->vertices, xml->nrmlquadstrip,xml->vertexcount, vertrw);
    SumNormals (xml->nrmllinstrip.size()/2,2,xml->vertices, xml->nrmllinstrip,xml->vertexcount, vertrw);
    delete []vertrw;
    for (i=0;i<xml->vertices.size();i++) {
      float dis = sqrtf (xml->vertices[i].i*xml->vertices[i].i +
			xml->vertices[i].j*xml->vertices[i].j +
			xml->vertices[i].k*xml->vertices[i].k);
      if (dis!=0) {
	xml->vertices[i].i/=dis;//renormalize
	xml->vertices[i].j/=dis;
	xml->vertices[i].k/=dis;
	fprintf (stderr, "Vertex %d, (%f,%f,%f) <%f,%f,%f>\n",i,
		 xml->vertices[i].x,
		 xml->vertices[i].y,
		 xml->vertices[i].z,
		 xml->vertices[i].i,
		 xml->vertices[i].j,
		 xml->vertices[i].k);
      }else {
	xml->vertices[i].i=xml->vertices[i].x;
	xml->vertices[i].j=xml->vertices[i].y;
	xml->vertices[i].k=xml->vertices[i].z;
	dis = sqrtf (xml->vertices[i].i*xml->vertices[i].i +
		     xml->vertices[i].j*xml->vertices[i].j +
		     xml->vertices[i].k*xml->vertices[i].k);
	if (dis!=0) {
	  xml->vertices[i].i/=dis;//renormalize
	  xml->vertices[i].j/=dis;
	  xml->vertices[i].k/=dis;	  
	}else {
	  xml->vertices[i].i=0;
	  xml->vertices[i].j=0;
	  xml->vertices[i].k=1;	  
	}
      } 
    }

    a=0;
    for (a=0;a<xml->tris.size();a+=3) {
      for (j=0;j<3;j++) {
	xml->tris[a+j].i = xml->vertices[xml->triind[a+j]].i;
	xml->tris[a+j].j = xml->vertices[xml->triind[a+j]].j;
	xml->tris[a+j].k = xml->vertices[xml->triind[a+j]].k;
      }
    }
    a=0;
    for (a=0;a<xml->quads.size();a+=4) {
      for (j=0;j<4;j++) {
	xml->quads[a+j].i=xml->vertices[xml->quadind[a+j]].i;
	xml->quads[a+j].j=xml->vertices[xml->quadind[a+j]].j;
	xml->quads[a+j].k=xml->vertices[xml->quadind[a+j]].k;
      }
    }
    a=0;
    unsigned int k=0;
    unsigned int l=0;
    for (l=a=0;a<xml->tristrips.size();a++) {
      for (k=0;k<xml->tristrips[a].size();k++,l++) {
	xml->tristrips[a][k].i = xml->vertices[xml->tristripind[l]].i;
	xml->tristrips[a][k].j = xml->vertices[xml->tristripind[l]].j;
	xml->tristrips[a][k].k = xml->vertices[xml->tristripind[l]].k;
      }
    }
    for (l=a=0;a<xml->trifans.size();a++) {
      for (k=0;k<xml->trifans[a].size();k++,l++) {
	xml->trifans[a][k].i = xml->vertices[xml->trifanind[l]].i;
	xml->trifans[a][k].j = xml->vertices[xml->trifanind[l]].j;
	xml->trifans[a][k].k = xml->vertices[xml->trifanind[l]].k;
      }
    }
    for (l=a=0;a<xml->quadstrips.size();a++) {
      for (k=0;k<xml->quadstrips[a].size();k++,l++) {
	xml->quadstrips[a][k].i = xml->vertices[xml->quadstripind[l]].i;
	xml->quadstrips[a][k].j = xml->vertices[xml->quadstripind[l]].j;
	xml->quadstrips[a][k].k = xml->vertices[xml->quadstripind[l]].k;
      }
    }

  }
  
  // TODO: add alpha handling

   //check for per-polygon flat shading
  unsigned int trimax = xml->tris.size()/3;
  unsigned int a=0;
  unsigned int i=0;
  unsigned int j=0;
  for (i=0;i<trimax;i++,a+=3) {
    if (xml->trishade[i]==1) {
      for (j=0;j<3;j++) {
	Vector Cur (xml->vertices[xml->triind[a+j]].x,
		    xml->vertices[xml->triind[a+j]].y,
		    xml->vertices[xml->triind[a+j]].z);
	Cur = (Vector (xml->vertices[xml->triind[a+((j+2)%3)]].x,
		       xml->vertices[xml->triind[a+((j+2)%3)]].y,
		       xml->vertices[xml->triind[a+((j+2)%3)]].z)-Cur)
	  .Cross(Vector (xml->vertices[xml->triind[a+((j+1)%3)]].x,
			 xml->vertices[xml->triind[a+((j+1)%3)]].y,
			 xml->vertices[xml->triind[a+((j+1)%3)]].z)-Cur);
	Normalize(Cur);
	//Cur = Cur*(1.00F/xml->vertexcount[a+j]);
	xml->tris[a+j].i=Cur.i/xml->vertexcount[xml->triind[a+j]];
	xml->tris[a+j].j=Cur.j/xml->vertexcount[xml->triind[a+j]];
	xml->tris[a+j].k=Cur.k/xml->vertexcount[xml->triind[a+j]];
      }
    }
  }
  a=0;
  trimax = xml->quads.size()/4;
  for (i=0;i<trimax;i++,a+=4) {
    if (xml->quadshade[i]==1) {
      for (j=0;j<4;j++) {
	Vector Cur (xml->vertices[xml->quadind[a+j]].x,
		    xml->vertices[xml->quadind[a+j]].y,
		    xml->vertices[xml->quadind[a+j]].z);
	Cur = (Vector (xml->vertices[xml->quadind[a+((j+2)%4)]].x,
		       xml->vertices[xml->quadind[a+((j+2)%4)]].y,
		       xml->vertices[xml->quadind[a+((j+2)%4)]].z)-Cur)
	  .Cross(Vector (xml->vertices[xml->quadind[a+((j+1)%4)]].x,
			 xml->vertices[xml->quadind[a+((j+1)%4)]].y,
			 xml->vertices[xml->quadind[a+((j+1)%4)]].z)-Cur);
	Normalize(Cur);
	//Cur = Cur*(1.00F/xml->vertexcount[a+j]);
	xml->quads[a+j].i=Cur.i/xml->vertexcount[xml->quadind[a+j]];
	xml->quads[a+j].j=Cur.j/xml->vertexcount[xml->quadind[a+j]];
	xml->quads[a+j].k=Cur.k/xml->vertexcount[xml->quadind[a+j]];
      }
    }
  }
  if (xml->decal_name.c_str()[0]=='\0') {	
    Decal = NULL;
  } else {
    if (xml->alpha_name.c_str()[0]=='\0') {
      Decal = new Texture(xml->decal_name.c_str(), 0);    
    }else {
      Decal = new Texture(xml->decal_name.c_str(), xml->alpha_name.c_str(),0);    
    }
  }

  int index = 0;

  unsigned int totalvertexsize = xml->tris.size()+xml->quads.size()+xml->lines.size();
  for (index=0;index<xml->tristrips.size();index++) {
    totalvertexsize += xml->tristrips[index].size();
  }
  for (index=0;index<xml->trifans.size();index++) {
    totalvertexsize += xml->trifans[index].size();
  }
  for (index=0;index<xml->quadstrips.size();index++) {
    totalvertexsize += xml->quadstrips[index].size();
  }
  for (index=0;index<xml->linestrips.size();index++) {
    totalvertexsize += xml->linestrips[index].size();
  }

  index =0;
  vertexlist = new GFXVertex[totalvertexsize];

  minSizeX = minSizeY = minSizeZ = FLT_MAX;
  maxSizeX = maxSizeY = maxSizeZ = -FLT_MAX;
  radialSize = 0;
  enum POLYTYPE * polytypes= new enum POLYTYPE[totalvertexsize];//overkill but what the hell
  int *poly_offsets  = new int [totalvertexsize];
  int o_index=0;
  if (xml->tris.size()) {
    polytypes[o_index]= GFXTRI;
    poly_offsets[o_index]=xml->tris.size();
    o_index++;
  }
  if (xml->quads.size()) {
    polytypes[o_index]=GFXQUAD;
    poly_offsets[o_index]=xml->quads.size();
    o_index++;
  }
  if (xml->lines.size()) {
    polytypes[o_index]=GFXLINE;
    poly_offsets[o_index]=xml->lines.size();
    o_index++;
  }
  /*
  if (xml->lines.size())
    polytypes[o_index]=GFXLINE;
    poly_offsets[o_index]=xml->lines.size()*2;  
    o_index++;
  */

  for(a=0; a<xml->tris.size(); a++, index++) {
    vertexlist[index] = xml->tris[a];		
    updateMax (minSizeX,maxSizeX,minSizeY,maxSizeY,minSizeZ,maxSizeZ,vertexlist[index]);
  }
  for(a=0; a<xml->quads.size(); a++, index++) {
    vertexlist[index] = xml->quads[a];
    updateMax (minSizeX,maxSizeX,minSizeY,maxSizeY,minSizeZ,maxSizeZ,vertexlist[index]);
  }
  for(a=0; a<xml->lines.size(); a++, index++) {
    vertexlist[index] = xml->lines[a];		
    updateMax (minSizeX,maxSizeX,minSizeY,maxSizeY,minSizeZ,maxSizeZ,vertexlist[index]);
  }

  for (a=0;a<xml->tristrips.size();a++) {

    for (int m=0;m<xml->tristrips[a].size();m++,index++) {
      vertexlist[index] = xml->tristrips[a][m];
    updateMax (minSizeX,maxSizeX,minSizeY,maxSizeY,minSizeZ,maxSizeZ,vertexlist[index]);
    }
    polytypes[o_index]= GFXTRISTRIP;
    poly_offsets[o_index]=xml->tristrips[a].size();
    o_index++;
  }
  for (a=0;a<xml->trifans.size();a++) {
    for (int m=0;m<xml->trifans[a].size();m++,index++) {
      vertexlist[index] = xml->trifans[a][m];
    updateMax (minSizeX,maxSizeX,minSizeY,maxSizeY,minSizeZ,maxSizeZ,vertexlist[index]);
    }
    polytypes[o_index]= GFXTRIFAN;
    poly_offsets[o_index]=xml->trifans[a].size();

    o_index++;
  }
  for (a=0;a<xml->quadstrips.size();a++) {
    for (int m=0;m<xml->quadstrips[a].size();m++,index++) {
      vertexlist[index] = xml->quadstrips[a][m];
    updateMax (minSizeX,maxSizeX,minSizeY,maxSizeY,minSizeZ,maxSizeZ,vertexlist[index]);
    }
    polytypes[o_index]= GFXQUADSTRIP;
    poly_offsets[o_index]=xml->quadstrips[a].size();
    o_index++;
  }
  for (a=0;a<xml->linestrips.size();a++) {

    for (int m=0;m<xml->linestrips[a].size();m++,index++) {
      vertexlist[index] = xml->linestrips[a][m];
      updateMax (minSizeX,maxSizeX,minSizeY,maxSizeY,minSizeZ,maxSizeZ,vertexlist[index]);
    }
    polytypes[o_index]= GFXLINESTRIP;
    poly_offsets[o_index]=xml->linestrips[a].size();
    o_index++;
  }
  minSizeX *=scale;
  maxSizeX *=scale;
  minSizeY *=scale;
  maxSizeY *=scale;
  minSizeZ *=scale;
  maxSizeZ *=scale;
  float x_center = (minSizeX + maxSizeX)/2.0,
    y_center = (minSizeY + maxSizeY)/2.0,
    z_center = (minSizeZ + maxSizeZ)/2.0;
  SetPosition(x_center, y_center, z_center);
  for(a=0; a<totalvertexsize; a++) {
    vertexlist[a].x*=scale;//FIXME
    vertexlist[a].y*=scale;
    vertexlist[a].z*=scale;

    vertexlist[a].x -= x_center;
    vertexlist[a].y -= y_center;
    vertexlist[a].z -= z_center;

  }

  minSizeX -= x_center;
  maxSizeX -= x_center;
  minSizeY -= y_center;
  maxSizeY -= y_center;
  minSizeZ -= z_center;
  maxSizeZ -= z_center;
  
  radialSize = .5*sqrtf ((maxSizeX-minSizeX)*(maxSizeX-minSizeX)+(maxSizeY-minSizeY)*(maxSizeY-minSizeY)+(maxSizeX-minSizeZ)*(maxSizeX-minSizeZ));


  vlist= new GFXVertexList(polytypes,totalvertexsize,vertexlist,o_index,poly_offsets); 

  /*
  vlist[GFXQUAD]= new GFXVertexList(GFXQUAD,xml->quads.size(),vertexlist+xml->tris.size());
  index = xml->tris.size()+xml->quads.size();
  numQuadstrips = xml->tristrips.size()+xml->trifans.size()+xml->quadstrips.size();
  quadstrips = new GFXVertexList* [numQuadstrips];
  unsigned int tmpind =0;
  for (a=0;a<xml->tristrips.size();a++,tmpind++) {
    quadstrips[tmpind]= new GFXVertexList (GFXTRISTRIP,xml->tristrips[a].size(),vertexlist+index);
    index+= xml->tristrips[a].size();
  }
  for (a=0;a<xml->trifans.size();a++,tmpind++) {
    quadstrips[tmpind]= new GFXVertexList (GFXTRIFAN,xml->trifans[a].size(),vertexlist+index);
    index+= xml->trifans[a].size();
  }
  for (a=0;a<xml->quadstrips.size();a++,tmpind++) {
    quadstrips[tmpind]= new GFXVertexList (GFXQUADSTRIP,xml->quadstrips[a].size(),vertexlist+index);
    index+= xml->quadstrips[a].size();
  }
  */

  
  //TODO: add force handling
  //Add Logos in:
  CreateLogos(x_center,y_center,z_center);
  // Calculate bounding sphere
  
  this->orig = oldmesh;
  *oldmesh=*this;
  oldmesh->orig = NULL;
  oldmesh->refcount++;
  if (minSizeX==FLT_MAX) {
    minSizeX = minSizeY = minSizeZ = 0;
    maxSizeX = maxSizeY = maxSizeZ = 0;
  }

  GFXSetMaterial (myMatNum,xml->material);

  delete [] vertexlist;
  delete []poly_offsets;
  delete xml;
}
void Mesh::CreateLogos(float x_center, float y_center, float z_center) {
  numforcelogo=numsquadlogo =0;
  int index;
  for (index=0;index<xml->logos.size();index++) {
    if (xml->logos[index].type==0)
      numforcelogo++;
    if (xml->logos[index].type==1)
      numsquadlogo++;
  }
  unsigned int nfl=numforcelogo;
  Logo ** tmplogo;
  Texture * Dec;
  for (index=0,nfl=numforcelogo,tmplogo=&forcelogos,Dec=_Universe->getForceLogo();index<2;index++,nfl=numsquadlogo,tmplogo=&squadlogos,Dec=_Universe->getSquadLogo()) {
    if (nfl==0)
      continue;
    Vector *PolyNormal = new Vector [nfl];
    Vector *center = new Vector [nfl];
    float *sizes = new float [nfl];
    float *rotations = new float [nfl];
    float *offset = new float [nfl];
    Vector *Ref = new Vector [nfl];
    Vector norm1,norm2,norm;
    int ri=0;
    for (unsigned int ind=0;ind<xml->logos.size();ind++) {
      if (xml->logos[ind].type==index) {
	float weight=0;
	norm2=Vector (xml->vertices[xml->logos[ind].refpnt[1]].x-
		      xml->vertices[xml->logos[ind].refpnt[0]].x,
		      xml->vertices[xml->logos[ind].refpnt[1]].y-
		      xml->vertices[xml->logos[ind].refpnt[0]].y,
		      xml->vertices[xml->logos[ind].refpnt[1]].z-
		      xml->vertices[xml->logos[ind].refpnt[0]].z);
	norm1=Vector (xml->vertices[xml->logos[ind].refpnt[2]].x-
		      xml->vertices[xml->logos[ind].refpnt[0]].x,
		      xml->vertices[xml->logos[ind].refpnt[2]].y-
		      xml->vertices[xml->logos[ind].refpnt[0]].y,
		      xml->vertices[xml->logos[ind].refpnt[2]].z-
		      xml->vertices[xml->logos[ind].refpnt[0]].z);
	CrossProduct (norm2,norm1,norm);
	
	Normalize(norm);//norm is our normal vect, norm1 is our reference vect
	Vector Cent(0,0,0);
	for (unsigned int rj=0;rj<xml->logos[ind].refpnt.size();rj++) {
	  weight+=xml->logos[ind].refweight[rj];
	  Cent += Vector (xml->vertices[xml->logos[ind].refpnt[rj]].x*xml->logos[ind].refweight[rj],
			  xml->vertices[xml->logos[ind].refpnt[rj]].y*xml->logos[ind].refweight[rj],
			  xml->vertices[xml->logos[ind].refpnt[rj]].z*xml->logos[ind].refweight[rj]);
	}	
	if (weight!=0) {
	  Cent.i/=weight;
	  Cent.j/=weight;
	  Cent.k/=weight;
	}
	//Cent.i-=x_center;
	//Cent.j-=y_center;
	//Cent.k-=z_center;
	Ref[ri]=norm2;
	PolyNormal[ri]=norm;
	center[ri] = Cent*scale-Vector (x_center,y_center,z_center);;
	sizes[ri]=xml->logos[ind].size*scale;
	rotations[ri]=xml->logos[ind].rotate;
	offset[ri]=xml->logos[ind].offset;
	ri++;
      }
    }
    *tmplogo= new Logo (nfl,center,PolyNormal,sizes,rotations, (float)0.01,Dec,Ref);
    delete [] Ref;
    delete []PolyNormal;
    delete []center;
    delete [] sizes;
    delete [] rotations;
    delete [] offset;
  }
}
