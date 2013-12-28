// The MIT License (MIT)
// 
// Copyright (c) 2013 James Vecore
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// Created by James Vecore
// james.vecore@gmail.com

#include <xsi_application.h>
#include <xsi_model.h>
#include <xsi_context.h>
#include <xsi_primitive.h>
#include <xsi_geometry.h>
#include <xsi_point.h>
#include <xsi_iceattribute.h>
#include <xsi_iceattributedataarray.h>
#include <xsi_vector2f.h>
#include <xsi_vector3f.h>
#include <xsi_vector4f.h>
#include <xsi_quaternionf.h>
#include <xsi_color4f.h>
#include <xsi_pluginregistrar.h>
#include <xsi_status.h>

using namespace XSI; 
using namespace XSI::MATH;

#include <map>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

#include <Partio.h>


struct DataMap
{
	Partio::ParticleAttributeType AttributeType;
	int Count;
};

static DataMap int1Map    = { Partio::INT   , 1 };
static DataMap float1Map  = { Partio::FLOAT , 1 };
static DataMap float2Map  = { Partio::FLOAT , 2 };
static DataMap float3Map  = { Partio::FLOAT , 3 };
static DataMap float4Map  = { Partio::FLOAT , 4 };
static DataMap vector3Map = { Partio::VECTOR, 3 };

static std::map<std::string, std::string> defaultChannelNameMap;
static std::map<siICENodeDataType, DataMap> defaultDataMapping;

void InitDefaults()
{
	if (defaultChannelNameMap.size() == 0)
	{
		// these are basically .PRT channels names by default, everything else should map 1:1 (i.e. ID -> ID automatically, Density -> Density etc)
		defaultChannelNameMap["PointPosition"] = "Position";
		defaultChannelNameMap["PointVelocity"] = "Velocity";
		defaultChannelNameMap["PointNormal"]   = "Normal";
		defaultChannelNameMap["PointUV"]       = "TextureCoord";
	}
	
	if (defaultDataMapping.size() == 0)
	{
		defaultDataMapping[siICENodeDataBool]       = int1Map;
		defaultDataMapping[siICENodeDataLong]       = int1Map;
		defaultDataMapping[siICENodeDataFloat]      = float1Map;
		defaultDataMapping[siICENodeDataVector2]    = float2Map;
		defaultDataMapping[siICENodeDataVector3]    = vector3Map;
		defaultDataMapping[siICENodeDataVector4]    = float4Map;
		defaultDataMapping[siICENodeDataQuaternion] = float4Map;
		defaultDataMapping[siICENodeDataColor4]     = float4Map;
		defaultDataMapping[siICENodeDataShape]      = int1Map;
		defaultDataMapping[siICENodeDataRotation]   = float4Map;
	}
}

// split grabbed from stackoverflow...
// http://stackoverflow.com/questions/236129/how-to-split-a-string-in-c
std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

// split grabbed from stackoverflow...
// http://stackoverflow.com/questions/236129/how-to-split-a-string-in-c
std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    return split(s, delim, elems);
}

// default implementation when the types match exactly or auto-convert without warning (i.e. bool -> int)
template <typename ArrayValueType,typename PrimativeType>
inline void StoreDataRaw(ArrayValueType& dataArrayValue, PrimativeType* partioDataPtr, int count = 1)
{
    partioDataPtr[0] = dataArrayValue;
}

template <>
inline void StoreDataRaw<CVector2f, float>(CVector2f& dataArrayValue, float* partioDataPtr, int count)
{
    partioDataPtr[0] = dataArrayValue.GetX();
    partioDataPtr[1] = dataArrayValue.GetY();
}

template <>
inline void StoreDataRaw<CVector3f, float>(CVector3f& dataArrayValue, float* partioDataPtr, int count)
{
    partioDataPtr[0] = dataArrayValue.GetX();
    partioDataPtr[1] = dataArrayValue.GetY();
    partioDataPtr[2] = dataArrayValue.GetZ();
}

template <>
inline void StoreDataRaw<CVector4f, float>(CVector4f& dataArrayValue, float* partioDataPtr, int count)
{
    partioDataPtr[0] = dataArrayValue.GetX();
    partioDataPtr[1] = dataArrayValue.GetY();
    partioDataPtr[2] = dataArrayValue.GetZ();
    partioDataPtr[3] = dataArrayValue.GetW();
}
template <>

inline void StoreDataRaw<CQuaternionf, float>(CQuaternionf& dataArrayValue, float* partioDataPtr, int count)
{
    partioDataPtr[0] = dataArrayValue.GetX();
    partioDataPtr[1] = dataArrayValue.GetY();
    partioDataPtr[2] = dataArrayValue.GetZ();
    partioDataPtr[3] = dataArrayValue.GetW();
}

template <>
inline void StoreDataRaw<CColor4f, float>(CColor4f& dataArrayValue, float* partioDataPtr, int count)
{
    partioDataPtr[0] = dataArrayValue.GetR();
    partioDataPtr[1] = dataArrayValue.GetG();
    partioDataPtr[2] = dataArrayValue.GetB();
	if (count > 3) // this is basically a special case for Krakatoa which does not support an alpha channel for color and is mapped to float3
	{
		partioDataPtr[3] = dataArrayValue.GetA();
	}
}

template <>
inline void StoreDataRaw<CShape, int>(CShape& dataArrayValue, int* partioDataPtr, int count)
{
    partioDataPtr[0] = dataArrayValue.GetType();
}

template <>
inline void StoreDataRaw<const CShape, int>(const CShape& dataArrayValue, int* partioDataPtr, int count)
{
	partioDataPtr[0] = dataArrayValue.GetType();
}

template <>
inline void StoreDataRaw<CRotationf,float>(CRotationf& dataArrayValue, float* partioDataPtr, int count)
{
    // For now this is just a default behavior that matches the current ice storage format
    // TODO: in the future this should be customizable per output type

    switch (dataArrayValue.GetRepresentation())
    {
    case CRotationf::siQuaternionRot:
        dataArrayValue.GetQuaternion().Get(partioDataPtr[3],partioDataPtr[0],partioDataPtr[1],partioDataPtr[2]);
        break;
    case CRotationf::siAxisAngleRot:
        dataArrayValue.GetAxisAngle(partioDataPtr[3]).Get(partioDataPtr[0],partioDataPtr[1],partioDataPtr[2]);
        break;
    case CRotationf::siEulerRot:
        dataArrayValue.GetXYZAngles().Get(partioDataPtr[0],partioDataPtr[1],partioDataPtr[2]);
        break;
    }
}

template <typename ArrayType,typename PrimativeType>
CStatus StoreData(ICEAttribute& iceAttr, Partio::ParticlesDataMutable* pPD, Partio::ParticleAttribute& partioAttr)
{
    // copy data from iceAttr to partioAttr

    ArrayType dataArray;
    iceAttr.GetDataArray(dataArray);

	int count = partioAttr.count;
    for (int i=0; i < pPD->numParticles(); i++)
    {
        PrimativeType* value = pPD->dataWrite<PrimativeType>(partioAttr, i);
        StoreDataRaw(dataArray[i], value, count);
    }
    
    return CStatus::OK;
}

CStatus ExportPrimitive(Primitive& prim, int frame, std::map<std::string,std::string>& channelNameMapping, std::map<siICENodeDataType,DataMap>& dataMapping, std::vector<std::string> channelsToExport, CString& outputFName)
{
    if (prim.GetType() != "pointcloud")
    {
        Application().LogMessage(L"Unsupported Primitive type: " + prim.GetType(),siErrorMsg);
        return CStatus::Abort;
    }

    CStatus res;
    
    Geometry geom = prim.GetGeometry((double)frame);
        
    CPointRefArray points( geom.GetPoints() );
    LONG particleCount = points.GetCount();

    Partio::ParticlesDataMutable* pPD = Partio::create();

    pPD->addParticles(particleCount);

    std::vector<std::string> skippedChanelWarnings;

    CRefArray attributesRefArray = geom.GetICEAttributes();
    for (int i=0; i < attributesRefArray.GetCount(); i++)
    {
        ICEAttribute attr(attributesRefArray[i]);
        if (attr.IsDefined() == false || attr.GetContextType() != siICENodeContextComponent0D)
            continue;

        siICENodeDataType dataType = attr.GetDataType();
        std::map<siICENodeDataType,DataMap>::const_iterator pos = dataMapping.find(dataType);
        if ( dataMapping.find(dataType) == dataMapping.end())
        {
            // unmapped data type, have to skip for now
            std::ostringstream ss;
            ss << "Skipping ICEAttribute: \"" << attr.GetName().GetAsciiString() << "\" due to unsupported data type: " << attr.GetDataType();
            skippedChanelWarnings.push_back(ss.str());
            continue;
        }
        DataMap dataMap = pos->second;

        
        // if (channelsToExport.size() == 0) then export all channels, else
        if (channelsToExport.size() != 0 && channelsToExport.end() == std::find(channelsToExport.begin(), channelsToExport.end(), attr.GetName().GetAsciiString()))
        {
            std::ostringstream ss;
            ss << "Skipping ICEAttribute: \"" << attr.GetName().GetAsciiString() << "\" because it was not selected for export";
            skippedChanelWarnings.push_back(ss.str());
            continue;
        }
            
        std::string destName (attr.GetName().GetAsciiString()); // need to store copy in std::string, otherwise the result sometimes gets lost
        std::map<std::string,std::string>::const_iterator namePos = channelNameMapping.find(attr.GetName().GetAsciiString());
        if (namePos != channelNameMapping.end())
        {
            destName = namePos->second;
        }
        
        Partio::ParticleAttribute partioAttr = pPD->addAttribute(destName.c_str(),dataMap.AttributeType, dataMap.Count);

		// don't try to access the data if the particle count is 0, it will just generate error 2392
		if (particleCount > 0)
		{

			switch (attr.GetDataType())
			{
			case siICENodeDataBool:
				StoreData<CICEAttributeDataArrayBool, int>(attr, pPD, partioAttr);
				break;
			case siICENodeDataLong:
				StoreData<CICEAttributeDataArrayLong, int>(attr, pPD, partioAttr);
				break;
			case siICENodeDataFloat:
				StoreData<CICEAttributeDataArrayFloat, float>(attr, pPD, partioAttr);
				break;
			case siICENodeDataVector2:
				StoreData<CICEAttributeDataArrayVector2f, float>(attr, pPD, partioAttr);
				break;
			case siICENodeDataVector3:
				StoreData<CICEAttributeDataArrayVector3f, float>(attr, pPD, partioAttr);
				break;
			case siICENodeDataVector4:
				StoreData<CICEAttributeDataArrayVector4f, float>(attr, pPD, partioAttr);
				break;
			case siICENodeDataQuaternion:
				StoreData<CICEAttributeDataArrayQuaternionf, float>(attr, pPD, partioAttr);
				break;
			case siICENodeDataColor4:
				StoreData<CICEAttributeDataArrayColor4f, float>(attr, pPD, partioAttr);
				break;
			case siICENodeDataShape:
				StoreData<CICEAttributeDataArrayShape, int>(attr, pPD, partioAttr);
				break;
			case siICENodeDataRotation:
				StoreData<CICEAttributeDataArrayRotationf, float>(attr, pPD, partioAttr);
				break;
			default:
				std::ostringstream ss;
				ss << "Skipping ICEAttribute: \"" << attr.GetName().GetAsciiString() << "\" due to unsupported data type";
				skippedChanelWarnings.push_back(ss.str());
				break;
			}
		}
    }

    for (std::vector<std::string>::const_iterator iter = skippedChanelWarnings.begin(); iter != skippedChanelWarnings.end(); ++iter)
    {
        Application().LogMessage(CString(iter->c_str()),siWarningMsg);    
    }

    Application().LogMessage(L"Writing " + CString(pPD->numParticles()) + " particles with " + CString(pPD->numAttributes()) + " channels to file at path: " + outputFName,siInfoMsg);
    Partio::write(outputFName.GetAsciiString(), *pPD, false);

    pPD->release(); // Partio memory release 

    return CStatus::OK;
}

CStatus DoParticleExport(CRef& in_ctxt, std::map<std::string,std::string>& channelNameMapping, std::map<siICENodeDataType,DataMap>& dataMapping)
{
    Context ctxt(in_ctxt);

    CString fileName(ctxt.GetAttribute(L"FileName"));
    CString fileType(ctxt.GetAttribute(L"FileType"));
    int frame = ctxt.GetAttribute(L"Frame");
        
	Application().LogMessage(L"Frame: "    + CString(ctxt.GetAttribute(L"Frame"))   ,siInfoMsg);
	Application().LogMessage(L"UserData: " + CString(ctxt.GetAttribute(L"UserData")),siInfoMsg);

    // if we are called from the cache manager, UserData will hold a comma separated list of Channels to export
    CValue userDataValue = ctxt.GetAttribute(L"UserData");
    bool exportSpecificChannels = false;
    std::vector<std::string> channelsToExport;
    if (userDataValue.m_t == CValue::siString)
    {
        split(std::string(CString(userDataValue).GetAsciiString()), ',',channelsToExport);
        exportSpecificChannels = true;   
    }
    
    CRef targetRef(ctxt.GetAttribute(L"Target"));
	
	// file extension must be lower case for partio to pickup the right exporter (LAME)
    int dotPos = fileName.ReverseFindString(".");
    CString fnameNoExt = fileName.GetSubString(0, dotPos);
    CString fnameExt   = fileName.GetSubString(dotPos);
    fnameExt.Lower(); 

	fileName = fnameNoExt + fnameExt;
    
    if (targetRef.GetClassID() == siPrimitiveID) // calls from cache manager look like this
    {
        Primitive prim(targetRef);
		return ExportPrimitive(prim, frame, channelNameMapping, dataMapping, channelsToExport, fileName);
    }
    else if (targetRef.GetClassID() == siModelID) // calls form model export look like  this
    {
        Model targetModel(targetRef);
        CRefArray& children = targetModel.GetChildren();

        for (int childIndex=0; childIndex < children.GetCount(); childIndex++)
        {
            CRef child(children[childIndex]);
            X3DObject exportTargetObject( child );
            Primitive prim = exportTargetObject.GetActivePrimitive();
			char buff[20];
			sprintf_s((char*)&buff, size_t(20), ".%04d", frame);
			CString frameStr(buff);
            CString outputFName = fnameNoExt + CString("_") + exportTargetObject.GetName() + frameStr + fnameExt;
            CStatus res = ExportPrimitive(prim, frame, channelNameMapping, dataMapping, channelsToExport, outputFName);
            if (res != CStatus::OK)
                return res;
        }
        return CStatus::OK; 
    }
    else
    {
        // expecting either a Model (File->Export->Model) or a Primitive for export (Cache Manager / CacheObjectsIntoFile)
        Application().LogMessage(L"Unsupported Target ClassID: " + CString(targetRef.GetClassIDName()),siErrorMsg); 
        return CStatus::Abort;
    }
}

SICALLBACK XSILoadPlugin(PluginRegistrar& in_reg)
{
	in_reg.PutAuthor(L"James Vecore");
	in_reg.PutEmail(L"james.vecore@gmail.com");
	in_reg.PutName(L"PartioParticleExporter Plug-in");
	in_reg.PutVersion(1, 0);
	in_reg.RegisterConverterEvent(L"CustomFileExportPRT", siOnCustomFileExport, "PRT");
	in_reg.RegisterConverterEvent(L"CustomFileExportBGEO", siOnCustomFileExport, "BGEO");
	in_reg.RegisterConverterEvent(L"CustomFileExportBIN", siOnCustomFileExport, "BIN");
	in_reg.RegisterConverterEvent(L"CustomFileExportGEO", siOnCustomFileExport, "GEO");
	in_reg.RegisterConverterEvent(L"CustomFileExportPDA", siOnCustomFileExport, "PDA");
	in_reg.RegisterConverterEvent(L"CustomFileExportPDB", siOnCustomFileExport, "PDB");
	in_reg.RegisterConverterEvent(L"CustomFileExportPDC", siOnCustomFileExport, "PDC");

	InitDefaults();

	//RegistrationInsertionPoint - do not remove this line

	return CStatus::OK;
}

SICALLBACK XSIUnloadPlugin(const PluginRegistrar& in_reg)
{
	CString strPluginName;
	strPluginName = in_reg.GetName();
	Application().LogMessage(strPluginName + L" has been unloaded.", siVerboseMsg);
	return CStatus::OK;
}

// Event callbacks for export

SICALLBACK CustomFileExportPRT_OnEvent(CRef& in_ctxt)
{
	InitDefaults(); // just in case these are not already initialized (i.e. if the dll was unloaded by softimage between calls)
		
	std::map<siICENodeDataType, DataMap> dataMapping = defaultDataMapping; // copy the defaults so we can add a special case for color
	// this is a special case for Krakatoa which expects 3 float color no alpha
    dataMapping[siICENodeDataColor4] = float3Map; 
	
    return DoParticleExport(in_ctxt, defaultChannelNameMap, dataMapping);
}

SICALLBACK CustomFileExportBGEO_OnEvent(CRef& in_ctxt)
{
	InitDefaults(); // just in case these are not already initialized (i.e. if the dll was unloaded by softimage between calls)

	std::map<std::string, std::string> channelNameMap;

	channelNameMap["PointPosition"]   = "position"; // this has to be 'position' and not 'P' or the bgeo writer for partio will fail (LAME)...
	channelNameMap["PointVelocity"]   = "v";
	channelNameMap["PointNormal"]     = "N";
	channelNameMap["PointUpVector"]   = "up";
	channelNameMap["PointUV"]         = "uv";
	channelNameMap["Color"]           = "Cd";
	channelNameMap["Mass"]            = "mass";
	channelNameMap["Size"]            = "pscale";
	channelNameMap["Scale"]           = "scalef";
	channelNameMap["DragCoefficient"] = "drag";
	
	std::map<siICENodeDataType, DataMap> dataMapping = defaultDataMapping; // copy the defaults so we can add a special case for color
	dataMapping[siICENodeDataColor4] = float3Map;

	return DoParticleExport(in_ctxt, channelNameMap, dataMapping);
}

SICALLBACK CustomFileExportGEO_OnEvent(CRef& in_ctxt)
{
	// same setup as the bgeo exporter, the extension will route to the proper writer
	return CustomFileExportBGEO_OnEvent(in_ctxt);
}

SICALLBACK CustomFileExportBIN_OnEvent(CRef& in_ctxt)
{
	InitDefaults(); // just in case these are not already initialized (i.e. if the dll was unloaded by softimage between calls)

	std::map<std::string, std::string> channelNameMap;

	// bin format partio writer expects very specific attribute names and only supports these channels
	channelNameMap["PointPosition"] = "position"; 
	channelNameMap["PointVelocity"] = "velocity";
	channelNameMap["PointNormal"]   = "normal";
	channelNameMap["Force"]         = "force";
	channelNameMap["Vorticity"]     = "vorticity";
	channelNameMap["Neighbors"]     = "neighbors";
	channelNameMap["PointUV"]       = "uvw";
	channelNameMap["Age"]           = "age";
	channelNameMap["IsolationTime"] = "isolationTime";
	channelNameMap["Viscosity"]     = "viscosity";
	channelNameMap["Density"]       = "density";
	channelNameMap["Pressure"]      = "pressure";
	channelNameMap["Mass"]          = "mass";
	channelNameMap["Temperature"  ] = "temperature";
	channelNameMap["ID"]            = "id";

	// the default data map should work just fine
	
	return DoParticleExport(in_ctxt, channelNameMap, defaultDataMapping);
}

SICALLBACK CustomFileExportPDA_OnEvent(CRef& in_ctxt)
{
	InitDefaults(); // just in case these are not already initialized (i.e. if the dll was unloaded by softimage between calls)
	
	std::map<std::string, std::string> channelNameMap;

	// guess based on this: http://download.autodesk.com/us/maya/2009help/index.html?url=Dynamics_Nodes_List_of_particle_attributes.htm,topicNumber=d0e413301
	// not really tested could use some feedback/pull requests here
	channelNameMap["PointPosition"] = "position"; // this has to be 'position' and not 'P' or the bgeo writer for partio will fail (LAME)...
	channelNameMap["PointVelocity"] = "velocity";
	channelNameMap["PointNormal"]   = "normalDir";
	channelNameMap["PointUpVector"] = "upDir";
	channelNameMap["Acceleration"]  = "acceleration";
	channelNameMap["Force"]         = "force";
	channelNameMap["PointUV"]       = "uv";
	channelNameMap["Color"]         = "objectColor";
	channelNameMap["Mass"]          = "mass";
	channelNameMap["Radius"]        = "radius";
	channelNameMap["Size"]          = "pointSize";
	channelNameMap["Age"]           = "age";
	channelNameMap["ID"]            = "id";

	std::map<siICENodeDataType, DataMap> dataMapping = defaultDataMapping; // copy the defaults so we can add a special case for color
	dataMapping[siICENodeDataColor4] = float3Map;
	
	return DoParticleExport(in_ctxt, channelNameMap, defaultDataMapping);
}

SICALLBACK CustomFileExportPDB_OnEvent(CRef& in_ctxt)
{
	return CustomFileExportPDA_OnEvent(in_ctxt); // should be the same as pda
}

SICALLBACK CustomFileExportPDC_OnEvent(CRef& in_ctxt)
{
	return CustomFileExportPDA_OnEvent(in_ctxt); // should be the same as pda
}



