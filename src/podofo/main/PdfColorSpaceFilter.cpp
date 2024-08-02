/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfColorSpaceFilter.h"
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfIndirectObjectList.h"

using namespace std;
using namespace PoDoFo;

PdfColorSpaceFilter::PdfColorSpaceFilter() { }

PdfColorSpaceFilter::~PdfColorSpaceFilter() { }

bool PdfColorSpaceFilter::IsRawEncoded() const
{
    return false;
}

bool PdfColorSpaceFilter::IsTrivial() const
{
    return false;
}

// TODO: pdfjs does some caching of the map based on object reference, we should do it as well
bool PdfColorSpaceFilterFactory::TryCreateFromObject(const PdfObject& obj, PdfColorSpaceFilterPtr& colorSpace)
{
    const PdfArray* arr;
    PdfColorSpaceType type;
    if (obj.TryGetArray(arr))
    {
        if (arr->GetSize() == 0)
        {
            PoDoFo::LogMessage(PdfLogSeverity::Warning, "Invalid color space");
            return false;
        }

        const PdfName* name;
        if (!arr->MustFindAt(0).TryGetName(name) || !PoDoFo::TryConvertTo(*name, type))
            return false;

        switch (type)
        {
            case PdfColorSpaceType::Indexed:
            {
                const PdfObjectStream* stream;
                charbuff lookup;
                int64_t maxIndex;
                PdfColorSpaceFilterPtr baseColorSpace;
                unsigned short componentCount;
                if (arr->GetSize() < 4)
                    goto InvalidIndexed; // Invalid array entry count

                if (!TryCreateFromObject(arr->MustFindAt(1), baseColorSpace))
                    goto InvalidIndexed;

                if (!arr->MustFindAt(2).TryGetNumber(maxIndex) && maxIndex < 1)
                    goto InvalidIndexed;

                stream = arr->MustFindAt(3).GetStream();
                if (stream == nullptr)
                    goto InvalidIndexed;

                switch (baseColorSpace->GetPixelFormat())
                {
                    case PdfColorSpacePixelFormat::RGB:
                        componentCount = 3;
                        break;
                    default:
                        PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedFilter, "Unsupported base color space in /Indexed color space");
                }

                lookup = stream->GetCopy();
                if (lookup.size() < componentCount * ((unsigned)maxIndex + 1))
                    goto InvalidIndexed;        // Table has invalid lookup map size

                colorSpace.reset(new PdfColorSpaceFilterIndexed(baseColorSpace, (unsigned)maxIndex + 1, std::move(lookup)));
                return true;

            InvalidIndexed:
                PoDoFo::LogMessage(PdfLogSeverity::Warning, "Invalid /Indexed color space name");
                return false;
            }
            default:
                PoDoFo::LogMessage(PdfLogSeverity::Warning, "Unsupported color space filter {}", name->GetString());
                return false;
        }
    }
    else
    {
        const PdfName* name;
        if (!obj.TryGetName(name) || !PoDoFo::TryConvertTo(name->GetString(), type))
            return false;

        switch (type)
        {
            case PdfColorSpaceType::DeviceGray:
            {
                colorSpace = GetDeviceGrayInstace();
                return true;
            }
            case PdfColorSpaceType::DeviceRGB:
            {
                colorSpace = GetDeviceRGBInstace();
                return true;
            }
            case PdfColorSpaceType::DeviceCMYK:
            {
                colorSpace = GetDeviceCMYKInstace();
                return true;
            }
            default:
            {
                PoDoFo::LogMessage(PdfLogSeverity::Warning, "Unsupported color space filter {}", name->GetString());
                return false;
            }
        }
    }
}

PdfColorSpaceFilterPtr PdfColorSpaceFilterFactory::GetTrivialFilter(PdfColorSpaceType type)
{
    switch (type)
    {
        case PdfColorSpaceType::DeviceRGB:
            return GetDeviceRGBInstace();
        case PdfColorSpaceType::DeviceGray:
            return GetDeviceGrayInstace();
        case PdfColorSpaceType::DeviceCMYK:
            return GetDeviceCMYKInstace();
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::CannotConvertColor, "Invalid color space");
    }
}

PdfColorSpaceFilterPtr PdfColorSpaceFilterFactory::GetUnkownInstance()
{
    static shared_ptr<PdfColorSpaceFilterUnkown> s_unknown(new PdfColorSpaceFilterUnkown());
    return s_unknown;
}

PdfColorSpaceFilterPtr PdfColorSpaceFilterFactory::GetDeviceGrayInstace()
{
    static shared_ptr<PdfColorSpaceDeviceGray> s_deviceGray(new PdfColorSpaceDeviceGray());
    return s_deviceGray;
}

PdfColorSpaceFilterPtr PdfColorSpaceFilterFactory::GetDeviceRGBInstace()
{
    static shared_ptr<PdfColorSpaceFilterDeviceRGB> s_deviceRGB(new PdfColorSpaceFilterDeviceRGB());
    return s_deviceRGB;
}

PdfColorSpaceFilterPtr PdfColorSpaceFilterFactory::GetDeviceCMYKInstace()
{
    static shared_ptr<PdfColorSpaceFilterDeviceCMYK> s_deviceCMYK(new PdfColorSpaceFilterDeviceCMYK());
    return s_deviceCMYK;
}

PdfColorSpaceDeviceGray::PdfColorSpaceDeviceGray() { }

PdfColorSpaceType PdfColorSpaceDeviceGray::GetType() const
{
    return PdfColorSpaceType::DeviceGray;
}

bool PdfColorSpaceDeviceGray::IsRawEncoded() const
{
    return true;
}

bool PdfColorSpaceDeviceGray::IsTrivial() const
{
    return true;
}

PdfColorSpacePixelFormat PdfColorSpaceDeviceGray::GetPixelFormat() const
{
    return PdfColorSpacePixelFormat::Grayscale;
}

unsigned PdfColorSpaceDeviceGray::GetSourceScanLineSize(unsigned width, unsigned bitsPerComponent) const
{
    return (width * bitsPerComponent + 8 - 1) / 8;
}

unsigned PdfColorSpaceDeviceGray::GetScanLineSize(unsigned width, unsigned bitsPerComponent) const
{
    return (width * bitsPerComponent + 8 - 1) / 8;
}

void PdfColorSpaceDeviceGray::FetchScanLine(unsigned char* dstScanLine, const unsigned char* srcScanLine, unsigned width, unsigned bitsPerComponent) const
{
    std::memcpy(dstScanLine, srcScanLine, width * bitsPerComponent / 8);
}

PdfObject PdfColorSpaceDeviceGray::GetExportObject(PdfIndirectObjectList& objects) const
{
    (void)objects;
    return PdfName("DeviceGray");
}

unsigned PdfColorSpaceDeviceGray::GetColorComponentCount() const
{
    return 1;
}

PdfColorSpaceFilterDeviceRGB::PdfColorSpaceFilterDeviceRGB() { }

PdfColorSpaceType PdfColorSpaceFilterDeviceRGB::GetType() const
{
    return PdfColorSpaceType::DeviceRGB;
}

bool PdfColorSpaceFilterDeviceRGB::IsRawEncoded() const
{
    return true;
}

bool PdfColorSpaceFilterDeviceRGB::IsTrivial() const
{
    return true;
}

PdfColorSpacePixelFormat PdfColorSpaceFilterDeviceRGB::GetPixelFormat() const
{
    return PdfColorSpacePixelFormat::RGB;
}

unsigned PdfColorSpaceFilterDeviceRGB::GetSourceScanLineSize(unsigned width, unsigned bitsPerComponent) const
{
    return (3 * width * bitsPerComponent + 8 - 1) / 8;
}

unsigned PdfColorSpaceFilterDeviceRGB::GetScanLineSize(unsigned width, unsigned bitsPerComponent) const
{
    return (3 * width * bitsPerComponent + 8 - 1) / 8;
}

void PdfColorSpaceFilterDeviceRGB::FetchScanLine(unsigned char* dstScanLine, const unsigned char* srcScanLine, unsigned width, unsigned bitsPerComponent) const
{
    std::memcpy(dstScanLine, srcScanLine, 3 * width * bitsPerComponent / 8);
}

PdfObject PdfColorSpaceFilterDeviceRGB::GetExportObject(PdfIndirectObjectList& objects) const
{
    (void)objects;
    return PdfName("DeviceRGB");
}

unsigned PdfColorSpaceFilterDeviceRGB::GetColorComponentCount() const
{
    return 3;
}

PdfColorSpaceFilterDeviceCMYK::PdfColorSpaceFilterDeviceCMYK() { }

PdfColorSpaceType PdfColorSpaceFilterDeviceCMYK::GetType() const
{
    return PdfColorSpaceType::DeviceCMYK;
}

bool PdfColorSpaceFilterDeviceCMYK::IsRawEncoded() const
{
    return true;
}

bool PdfColorSpaceFilterDeviceCMYK::IsTrivial() const
{
    return true;
}

PdfColorSpacePixelFormat PdfColorSpaceFilterDeviceCMYK::GetPixelFormat() const
{
    return PdfColorSpacePixelFormat::CMYK;
}

unsigned PdfColorSpaceFilterDeviceCMYK::GetSourceScanLineSize(unsigned width, unsigned bitsPerComponent) const
{
    return (4 * width * bitsPerComponent + 8 - 1) / 8;
}

unsigned PdfColorSpaceFilterDeviceCMYK::GetScanLineSize(unsigned width, unsigned bitsPerComponent) const
{
    return (4 * width * bitsPerComponent + 8 - 1) / 8;
}

void PdfColorSpaceFilterDeviceCMYK::FetchScanLine(unsigned char* dstScanLine, const unsigned char* srcScanLine, unsigned width, unsigned bitsPerComponent) const
{
    std::memcpy(dstScanLine, srcScanLine, 4 * width * bitsPerComponent / 8);
}

PdfObject PdfColorSpaceFilterDeviceCMYK::GetExportObject(PdfIndirectObjectList& objects) const
{
    (void)objects;
    return PdfName("DeviceCMYK");
}

unsigned PdfColorSpaceFilterDeviceCMYK::GetColorComponentCount() const
{
    return 4;
}

PdfColorSpaceFilterIndexed::PdfColorSpaceFilterIndexed(const PdfColorSpaceFilterPtr& baseColorSpace, unsigned mapSize, charbuff&& lookup)
    : m_BaseColorSpace(baseColorSpace), m_MapSize(mapSize), m_lookup(std::move(lookup))
{
}

PdfColorSpaceType PdfColorSpaceFilterIndexed::GetType() const
{
    return PdfColorSpaceType::Indexed;
}

PdfColorSpacePixelFormat PdfColorSpaceFilterIndexed::GetPixelFormat() const
{
    return m_BaseColorSpace->GetPixelFormat();
}

unsigned PdfColorSpaceFilterIndexed::GetSourceScanLineSize(unsigned width, unsigned bitsPerComponent) const
{
    // bitsPerComponent Ignored in /Indexed source scan line size. The "lookup" table
    // always map to color components that are 8 bits size long
    (void)bitsPerComponent;
    return width;
}

unsigned PdfColorSpaceFilterIndexed::GetScanLineSize(unsigned width, unsigned bitsPerComponent) const
{
    switch (m_BaseColorSpace->GetPixelFormat())
    {
        case PdfColorSpacePixelFormat::RGB:
            return (3 * width * bitsPerComponent + 8 - 1) / 8;
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedFilter, "Unsupported base color space in /Indexed color space");
    }
}

void PdfColorSpaceFilterIndexed::FetchScanLine(unsigned char* dstScanLine, const unsigned char* srcScanLine, unsigned width, unsigned bitsPerComponent) const
{
    switch (m_BaseColorSpace->GetType())
    {
        case PdfColorSpaceType::DeviceRGB:
        {
            if (bitsPerComponent == 8)
            {
                for (unsigned i = 0; i < width; i++)
                {
                    PODOFO_INVARIANT(srcScanLine[i] < m_MapSize);
                    const unsigned char* mappedColor = (const unsigned char*)(m_lookup.data() + srcScanLine[i] * 3);
                    *(dstScanLine + i * 3 + 0) = mappedColor[0];
                    *(dstScanLine + i * 3 + 1) = mappedColor[1];
                    *(dstScanLine + i * 3 + 2) = mappedColor[2];
                }
            }
            else
            {
                PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedFilter, "/BitsPerComponent != 8");
            }
            break;
        }
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedFilter, "Unsupported base color space in /Indexed color space");
    }


}

PdfObject PdfColorSpaceFilterIndexed::GetExportObject(PdfIndirectObjectList& objects) const
{
    auto& lookupObj = objects.CreateDictionaryObject();
    lookupObj.GetOrCreateStream().SetData(m_lookup);

    PdfArray arr;
    arr.Add(PdfName("Indexed"));
    arr.Add(m_BaseColorSpace->GetExportObject(objects));
    arr.Add(static_cast<int64_t>(m_MapSize - 1));
    arr.Add(lookupObj.GetIndirectReference());
    return arr;
}

unsigned PdfColorSpaceFilterIndexed::GetColorComponentCount() const
{
    return 1;
}

PdfColorSpaceFilterUnkown::PdfColorSpaceFilterUnkown() { }

PdfColorSpaceType PdfColorSpaceFilterUnkown::GetType() const
{
    return PdfColorSpaceType::Unknown;
}

PdfColorSpacePixelFormat PdfColorSpaceFilterUnkown::GetPixelFormat() const
{
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Operation unsupported in unknown type color space");
}

unsigned PdfColorSpaceFilterUnkown::GetSourceScanLineSize(unsigned width, unsigned bitsPerComponent) const
{
    (void)width;
    (void)bitsPerComponent;
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Operation unsupported in unknown type color space");
}

unsigned PdfColorSpaceFilterUnkown::GetScanLineSize(unsigned width, unsigned bitsPerComponent) const
{
    (void)width;
    (void)bitsPerComponent;
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Operation unsupported in unknown type color space");
}

void PdfColorSpaceFilterUnkown::FetchScanLine(unsigned char* dstScanLine, const unsigned char* srcScanLine, unsigned width, unsigned bitsPerComponent) const
{
    (void)dstScanLine;
    (void)srcScanLine;
    (void)width;
    (void)bitsPerComponent;
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Operation unsupported in unknown type color space");
}

PdfObject PdfColorSpaceFilterUnkown::GetExportObject(PdfIndirectObjectList& objects) const
{
    (void)objects;
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Operation unsupported in unknown type color space");
}

unsigned PdfColorSpaceFilterUnkown::GetColorComponentCount() const
{
    PODOFO_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Operation unsupported in unknown type color space");
}

PdfColorSpaceFilterSeparation::PdfColorSpaceFilterSeparation(const string_view& name, const PdfColor& alternateColor)
    : m_Name(name), m_AlternateColor(alternateColor)
{
    switch (alternateColor.GetColorSpace())
    {
        case PdfColorSpaceType::DeviceGray:
        case PdfColorSpaceType::DeviceRGB:
        case PdfColorSpaceType::DeviceCMYK:
            break;
        default:
            PODOFO_RAISE_ERROR_INFO(PdfErrorCode::CannotConvertColor, "Unsupported color space for color space separation");
    }
}

unique_ptr<PdfColorSpaceFilterSeparation> PdfColorSpaceFilterSeparation::CreateSeparationNone()
{
    return unique_ptr<PdfColorSpaceFilterSeparation>(new PdfColorSpaceFilterSeparation("None", PdfColor(0, 0, 0, 0)));
}

unique_ptr<PdfColorSpaceFilterSeparation> PdfColorSpaceFilterSeparation::CreateSeparationAll()
{
    return unique_ptr<PdfColorSpaceFilterSeparation>(new PdfColorSpaceFilterSeparation("All", PdfColor(1,1,1,1)));
}

PdfColorSpaceType PdfColorSpaceFilterSeparation::GetType() const
{
    return PdfColorSpaceType::Separation;
}

bool PdfColorSpaceFilterSeparation::IsRawEncoded() const
{
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

PdfColorSpacePixelFormat PdfColorSpaceFilterSeparation::GetPixelFormat() const
{
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

unsigned PdfColorSpaceFilterSeparation::GetSourceScanLineSize(unsigned width, unsigned bitsPerComponent) const
{
    (void)width;
    (void)bitsPerComponent;
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

unsigned PdfColorSpaceFilterSeparation::GetScanLineSize(unsigned width, unsigned bitsPerComponent) const
{
    (void)width;
    (void)bitsPerComponent;
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

void PdfColorSpaceFilterSeparation::FetchScanLine(unsigned char* dstScanLine, const unsigned char* srcScanLine, unsigned width, unsigned bitsPerComponent) const
{
    (void)dstScanLine;
    (void)srcScanLine;
    (void)width;
    (void)bitsPerComponent;
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

PdfObject PdfColorSpaceFilterSeparation::GetExportObject(PdfIndirectObjectList& objects) const
{
    // Build color-spaces for separation
    auto& csTintFunc = objects.CreateDictionaryObject();
    
    csTintFunc.GetDictionary().AddKey("BitsPerSample", static_cast<int64_t>(8));
    
    PdfArray decode;
    decode.Add(static_cast<int64_t>(0));
    decode.Add(static_cast<int64_t>(1));
    decode.Add(static_cast<int64_t>(0));
    decode.Add(static_cast<int64_t>(1));
    decode.Add(static_cast<int64_t>(0));
    decode.Add(static_cast<int64_t>(1));
    decode.Add(static_cast<int64_t>(0));
    decode.Add(static_cast<int64_t>(1));
    csTintFunc.GetDictionary().AddKey("Decode", decode);
    
    PdfArray domain;
    domain.Add(static_cast<int64_t>(0));
    domain.Add(static_cast<int64_t>(1));
    csTintFunc.GetDictionary().AddKey("Domain", domain);
    
    PdfArray encode;
    encode.Add(static_cast<int64_t>(0));
    encode.Add(static_cast<int64_t>(1));
    csTintFunc.GetDictionary().AddKey("Encode", encode);
    
    csTintFunc.GetDictionary().AddKey(PdfNames::Filter, PdfName("FlateDecode"));
    csTintFunc.GetDictionary().AddKey("FunctionType", PdfVariant(static_cast<int64_t>(0)));
    //csTintFunc->GetDictionary().AddKey( "FunctionType",
    //                                    PdfVariant( static_cast<int64_t>(EPdfFunctionType::Sampled) ) );
    
    switch (m_AlternateColor.GetColorSpace())
    {
        case PdfColorSpaceType::DeviceGray:
        {
            char data[1 * 2];
            data[0] = 0;
            data[1] = static_cast<char> (m_AlternateColor.GetGrayScale());
    
            PdfArray range;
            range.Add(static_cast<int64_t>(0));
            range.Add(static_cast<int64_t>(1));
            csTintFunc.GetDictionary().AddKey("Range", range);
    
            PdfArray size;
            size.Add(static_cast<int64_t>(2));
            csTintFunc.GetDictionary().AddKey("Size", size);
    
            csTintFunc.GetOrCreateStream().SetData(bufferview(data, 1 * 2));
    
            PdfArray csArr;
            csArr.Add(PdfName("Separation"));
            csArr.Add(PdfName(m_Name));
            csArr.Add(PdfName("DeviceGray"));
            csArr.Add(csTintFunc.GetIndirectReference());
            return csArr;
        }
        case PdfColorSpaceType::DeviceRGB:
        {
            char data[3 * 2];
            data[0] = 0;
            data[1] = 0;
            data[2] = 0;
            data[3] = static_cast<char> (m_AlternateColor.GetRed() * 255);
            data[4] = static_cast<char> (m_AlternateColor.GetGreen() * 255);
            data[5] = static_cast<char> (m_AlternateColor.GetBlue() * 255);
    
            PdfArray range;
            range.Add(static_cast<int64_t>(0));
            range.Add(static_cast<int64_t>(1));
            range.Add(static_cast<int64_t>(0));
            range.Add(static_cast<int64_t>(1));
            range.Add(static_cast<int64_t>(0));
            range.Add(static_cast<int64_t>(1));
            csTintFunc.GetDictionary().AddKey("Range", range);
    
            PdfArray size;
            size.Add(static_cast<int64_t>(2));
            csTintFunc.GetDictionary().AddKey("Size", size);
    
            csTintFunc.GetOrCreateStream().SetData(bufferview(data, 3 * 2));
    
            PdfArray csArr;
            csArr.Add(PdfName("Separation"));
            csArr.Add(PdfName(m_Name));
            csArr.Add(PdfName("DeviceRGB"));
            csArr.Add(csTintFunc.GetIndirectReference());
            return csArr;
        }
        case PdfColorSpaceType::DeviceCMYK:
        {
            char data[4 * 2];
            data[0] = 0;
            data[1] = 0;
            data[2] = 0;
            data[3] = 0;
            data[4] = static_cast<char> (m_AlternateColor.GetCyan() * 255);
            data[5] = static_cast<char> (m_AlternateColor.GetMagenta() * 255);
            data[6] = static_cast<char> (m_AlternateColor.GetYellow() * 255);
            data[7] = static_cast<char> (m_AlternateColor.GetBlack() * 255);
    
            PdfArray range;
            range.Add(static_cast<int64_t>(0));
            range.Add(static_cast<int64_t>(1));
            range.Add(static_cast<int64_t>(0));
            range.Add(static_cast<int64_t>(1));
            range.Add(static_cast<int64_t>(0));
            range.Add(static_cast<int64_t>(1));
            range.Add(static_cast<int64_t>(0));
            range.Add(static_cast<int64_t>(1));
            csTintFunc.GetDictionary().AddKey("Range", range);
    
            PdfArray size;
            size.Add(static_cast<int64_t>(2));
            csTintFunc.GetDictionary().AddKey("Size", size);
    
            PdfArray csArr;
            csArr.Add(PdfName("Separation"));
            csArr.Add(PdfName(m_Name));
            csArr.Add(PdfName("DeviceCMYK"));
            csArr.Add(csTintFunc.GetIndirectReference());
    
            csTintFunc.GetOrCreateStream().SetData(bufferview(data, 4 * 2)); // set stream as last, so that it will work with PdfStreamedDocument
            return csArr;
        }

        case PdfColorSpaceType::Unknown:
        default:
        {
            PODOFO_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
        }
    }
}

unsigned PdfColorSpaceFilterSeparation::GetColorComponentCount() const
{
    return 1;
}

PdfColorSpaceFilterLab::PdfColorSpaceFilterLab(const array<double, 3>& whitePoint,
        nullable<const array<double, 3>&> blackPoint,
        nullable<const array<double, 4>&> range) :
    m_WhitePoint(whitePoint),
    m_BlackPoint(blackPoint == nullptr ? array<double, 3>{ } : *blackPoint),
    m_Range(range == nullptr ? array<double, 4>{ -100, 100, -100, 100 } : *range)
{
}

PdfColorSpaceType PdfColorSpaceFilterLab::GetType() const
{
    return PdfColorSpaceType::Lab;
}

bool PdfColorSpaceFilterLab::IsRawEncoded() const
{
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

PdfColorSpacePixelFormat PdfColorSpaceFilterLab::GetPixelFormat() const
{
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

unsigned PdfColorSpaceFilterLab::GetSourceScanLineSize(unsigned width, unsigned bitsPerComponent) const
{
    (void)width;
    (void)bitsPerComponent;
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

unsigned PdfColorSpaceFilterLab::GetScanLineSize(unsigned width, unsigned bitsPerComponent) const
{
    (void)width;
    (void)bitsPerComponent;
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

void PdfColorSpaceFilterLab::FetchScanLine(unsigned char* dstScanLine, const unsigned char* srcScanLine, unsigned width, unsigned bitsPerComponent) const
{
    (void)dstScanLine;
    (void)srcScanLine;
    (void)width;
    (void)bitsPerComponent;
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

PdfObject PdfColorSpaceFilterLab::GetExportObject(PdfIndirectObjectList& objects) const
{
    auto& labDict = objects.CreateDictionaryObject().GetDictionary();
    PdfArray arr;
    arr.Add(m_WhitePoint[0]);
    arr.Add(m_WhitePoint[1]);
    arr.Add(m_WhitePoint[2]);
    labDict.AddKey("WhitePoint", arr);

    if (m_BlackPoint != array<double, 3>{ })
    {
        arr.Clear();
        arr.Add(m_BlackPoint[0]);
        arr.Add(m_BlackPoint[1]);
        arr.Add(m_BlackPoint[2]);
        labDict.AddKey("BlackPoint", arr);
    }

    if (m_Range != array<double, 4>{ -100, 100, -100, 100 })
    {
        arr.Clear();
        arr.Add(m_Range[0]);
        arr.Add(m_Range[1]);
        arr.Add(m_Range[2]);
        arr.Add(m_Range[3]);
        labDict.AddKey("Range", arr);
    }

    PdfArray labArr;
    labArr.Add(PdfName("Lab"));
    labArr.Add(labDict);
    return labArr;
}

unsigned PdfColorSpaceFilterLab::GetColorComponentCount() const
{
    return 3;
}
