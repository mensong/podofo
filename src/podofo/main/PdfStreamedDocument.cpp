/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2023 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <podofo/private/PdfDeclarationsPrivate.h>
#include "PdfStreamedDocument.h"
#include <podofo/auxiliary/StreamDevice.h>
#include "PdfImmediateWriter.h"

using namespace std;
using namespace PoDoFo;

PdfStreamedDocument::PdfStreamedDocument(const shared_ptr<OutputStreamDevice>& device, PdfVersion version,
        PdfEncrypt* encrypt, PdfSaveOptions opts) :
    m_Device(device),
    m_Encrypt(encrypt)
{
    init(version, opts);
}

PdfStreamedDocument::PdfStreamedDocument(const string_view& filename, PdfVersion version,
        PdfEncrypt* encrypt, PdfSaveOptions opts) :
    m_Device(new FileStreamDevice(filename, FileMode::Create)),
    m_Encrypt(encrypt)
{
    init(version, opts);
}

PdfStreamedDocument::~PdfStreamedDocument()
{
    GetFonts().EmbedFonts();
}

void PdfStreamedDocument::init(PdfVersion version, PdfSaveOptions opts)
{
    m_Writer.reset(new PdfImmediateWriter(this->GetObjects(), this->GetTrailer().GetObject(), *m_Device, version, m_Encrypt, opts));
}

PdfVersion PdfStreamedDocument::GetPdfVersion() const
{
    return m_Writer->GetPdfVersion();
}

void PdfStreamedDocument::SetPdfVersion(PdfVersion version)
{
    (void)version;
    PODOFO_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

const PdfEncrypt* PdfStreamedDocument::GetEncrypt() const
{
    return m_Encrypt;
}
