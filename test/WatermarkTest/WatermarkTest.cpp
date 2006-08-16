/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "PdfAction.h"
#include "PdfAnnotation.h"
#include "PdfDocument.h"
#include "PdfFont.h"
#include "PdfFontMetrics.h"
#include "PdfImage.h"
#include "PdfPage.h"
#include "PdfPainter.h"
#include "PdfRect.h"
#include "PdfString.h"
#include "PdfXObject.h"

#include "../PdfTest.h"

using namespace PoDoFo;

void WatermarkFile( const char* pszInFilename, const char* pszOutFilename )
{
    printf("Running watermark test\n");

    PdfDocument doc( pszInFilename );
    PdfPainter  painter;
    PdfPage*    pPage;
    PdfRect     rect;
    int         i;

    for(i=0;i<doc.GetPageCount();i++)
    {
        pPage = doc.GetPage( i );
        if( !pPage ) 
        {
            RAISE_ERROR( ePdfError_InvalidHandle );
        }
        
        rect = pPage->PageSize();

        painter.SetPage( pPage );
        painter.SetStrokingColor( 1.0, 0.0, 0.0 );
        painter.DrawLine( 0.0, 0.0, rect.Width(), rect.Height() );
    }

    printf("writing document back\n");
    doc.Write( pszOutFilename );
}

int main( int argc, char* argv[] ) 
{
    if( argc != 3 )
    {
        printf("Usage: WatermarkTest input_filename output_filename\n");
        return 0;
    }

    printf("This test tests the PdfWriter and PdfDocument classes.\n");
    printf("It opens an existing PDF and draws an X on each page.\n");
    printf("---\n");

    printf("Watermarking....\n");
    try {
        WatermarkFile( argv[1], argv[2] );
    } 
    catch( const PdfError & e ) 
    {
        e.PrintErrorMsg();
        return e.Error();
    }

    return 0;
}
