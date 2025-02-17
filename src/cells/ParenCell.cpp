// -*- mode: c++; c-file-style: "linux"; c-basic-offset: 2; indent-tabs-mode:
// nil -*-
//
//  Copyright (C) 2004-2015 Andrej Vodopivec <andrej.vodopivec@gmail.com>
//            (C) 2014-2018 Gunter Königsmann <wxMaxima@physikbuch.de>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
//
//  SPDX-License-Identifier: GPL-2.0+

/*! \file
  This file defines the class ParenCell

  ParenCell is the Cell type that represents a math element that is kept
  between parenthesis.
*/

#include "ParenCell.h"
#include "CellImpl.h"
#include "VisiblyInvalidCell.h"

ParenCell::ParenCell(GroupCell *group, Configuration *config,
                     std::unique_ptr<Cell> &&inner)
  : Cell(group, config),
    m_open(std::make_unique<TextCell>(group, config, wxT("("))),
    m_innerCell(std::move(inner)),
    m_close(std::make_unique<TextCell>(group, config, wxT(")"))) {
  InitBitFields();
  if (!m_innerCell)
    m_innerCell = std::make_unique<VisiblyInvalidCell>(group, config);
  m_innerCell->SetSuppressMultiplicationDot(true);
  m_open->SetStyle(TS_FUNCTION);
  m_close->SetStyle(TS_FUNCTION);
}

// These false-positive warnings only appear in old versions of cppcheck
// that don't fully understand constructor delegation, still.
// cppcheck-suppress uninitMemberVar symbolName=ParenCell::m_last1
// cppcheck-suppress uninitMemberVar symbolName=ParenCell::m_print
// cppcheck-suppress uninitMemberVar symbolName=ParenCell::m_numberOfExtensions
// cppcheck-suppress uninitMemberVar symbolName=ParenCell::m_charWidth1
// cppcheck-suppress uninitMemberVar symbolName=ParenCell::m_charHeight1
// cppcheck-suppress uninitMemberVar symbolName=ParenCell::m_signWidth
// cppcheck-suppress uninitMemberVar symbolName=ParenCell::m_signHeight
// cppcheck-suppress uninitMemberVar symbolName=ParenCell::m_signTopHeight
// cppcheck-suppress uninitMemberVar symbolName=ParenCell::m_signBotHeight
// cppcheck-suppress uninitMemberVar symbolName=ParenCell::m_extendHeight
ParenCell::ParenCell(GroupCell *group, const ParenCell &cell)
  : ParenCell(group, cell.m_configuration,
	      CopyList(group, cell.m_innerCell.get())) {
  CopyCommonData(cell);
}

DEFINE_CELL(ParenCell)

void ParenCell::SetInner(std::unique_ptr<Cell> inner, CellType type) {
  if (!inner)
    return;
  m_innerCell = std::move(inner);

  m_type = type;
  // Tell the first of our inner cells not to begin with a multiplication dot.
  m_innerCell->SetSuppressMultiplicationDot(true);
  ResetSize();
}

void ParenCell::SetFont(AFontSize fontsize) {
  wxASSERT(fontsize.IsValid());

  wxDC *dc = m_configuration->GetDC();

  Style style;
  if (m_bigParenType == Configuration::ascii)
    style = m_configuration->GetStyle(TS_FUNCTION, fontsize);
  else
    style = m_configuration->GetStyle(TS_FUNCTION,
                                      m_configuration->GetMathFontSize());

  wxASSERT(style.GetFontSize().IsValid());

  switch (m_bigParenType) {
  case Configuration::ascii:
  case Configuration::assembled_unicode:
    break;

  case Configuration::assembled_unicode_fallbackfont:
    style.SetFontName(AFontName::Linux_Libertine());
    break;

  case Configuration::assembled_unicode_fallbackfont2:
    style.SetFontName(AFontName::Linux_Libertine_O());
    break;

  default:
    break;
  }

  style.Italic(false).Underlined(false);

  if (!style.IsFontOk()) {
    style.SetFamily(wxFONTFAMILY_MODERN);
    style.SetFontName({});
  }

  if (!style.IsFontOk())
    style = Style::FromStockFont(wxStockGDI::FONT_NORMAL);

  // A fallback if we have been completely unable to set a working font
  if (!dc->GetFont().IsOk())
    m_bigParenType = Configuration::handdrawn;

  if (m_bigParenType != Configuration::handdrawn)
    dc->SetFont(style.GetFont());

  SetForeground();
}

void ParenCell::Recalculate(AFontSize fontsize) {
  m_innerCell->RecalculateList(fontsize);
  m_open->RecalculateList(fontsize);
  m_close->RecalculateList(fontsize);

  wxDC *dc = m_configuration->GetDC();
  int size = m_innerCell->GetHeightList();
  auto fontsize1 = Scale_Px(fontsize);
  // If our font provides all the unicode chars we need we don't need
  // to bother which exotic method we need to use for drawing nice parenthesis.
  if (fontsize1 * 3 > size) {
    if (m_configuration->GetParenthesisDrawMode() != Configuration::handdrawn)
      m_bigParenType = Configuration::ascii;
    m_signHeight = m_open->GetHeightList();
    m_signWidth = m_open->GetWidth();
  } else {
    m_bigParenType = m_configuration->GetParenthesisDrawMode();
    if (m_bigParenType != Configuration::handdrawn) {
      SetFont(fontsize);
      int signWidth1, signWidth2, signWidth3, descent, leading;
      dc->GetTextExtent(wxT(PAREN_OPEN_TOP_UNICODE), &signWidth1,
                        &m_signTopHeight, &descent, &leading);
      m_signTopHeight -= 2 * descent + Scale_Px(1);
      dc->GetTextExtent(wxT(PAREN_OPEN_EXTEND_UNICODE), &signWidth2,
                        &m_extendHeight, &descent, &leading);
      m_extendHeight -= 2 * descent + Scale_Px(1);
      dc->GetTextExtent(wxT(PAREN_OPEN_BOTTOM_UNICODE), &signWidth3,
                        &m_signBotHeight, &descent, &leading);
      m_signBotHeight -= descent + Scale_Px(1);

      m_signWidth = signWidth1;
      if (m_signWidth < signWidth2)
        m_signWidth = signWidth2;
      if (m_signWidth < signWidth3)
        m_signWidth = signWidth3;

      if (m_extendHeight < 1)
        m_extendHeight = 1;

      m_numberOfExtensions =
	((size - m_signTopHeight - m_signBotHeight + m_extendHeight / 2 - 1) /
	 m_extendHeight);
      if (m_numberOfExtensions < 0)
        m_numberOfExtensions = 0;
      m_signHeight = m_signTopHeight + m_signBotHeight +
	m_extendHeight * m_numberOfExtensions;
    } else {
      m_signWidth = Scale_Px(6) + m_configuration->GetDefaultLineWidth();
      if (m_signWidth < size / 15)
        m_signWidth = size / 15;
    }
  }
  m_width = m_innerCell->GetFullWidth() + m_signWidth * 2;
  if (IsBrokenIntoLines())
    m_width = 0;

  m_height = wxMax(m_signHeight, m_innerCell->GetHeightList()) + Scale_Px(2);
  m_center = m_height / 2;

  dc->GetTextExtent(wxT("("), &m_charWidth1, &m_charHeight1);
  if (m_charHeight1 < 2)
    m_charHeight1 = 2;

  if (IsBrokenIntoLines()) {
    m_height = wxMax(m_innerCell->GetHeightList(), m_open->GetHeightList());
    m_center = wxMax(m_innerCell->GetCenterList(), m_open->GetCenterList());
  } else {
    if (m_innerCell) {
      switch (m_bigParenType) {
      case Configuration::ascii:
        m_signHeight = m_charHeight1;
        break;
      case Configuration::assembled_unicode:
      case Configuration::assembled_unicode_fallbackfont:
      case Configuration::assembled_unicode_fallbackfont2:
        // Center the contents of the parenthesis vertically.
        //  m_innerCell->m_currentPoint.y += m_center - m_signHeight / 2;
        break;
      default: {
      }
      }
      m_innerCell->SetCurrentPoint(
				   wxPoint(m_currentPoint.x + m_signWidth, m_currentPoint.y));

      // Center the argument of all big parenthesis vertically
      if (m_bigParenType != Configuration::ascii)
        m_innerCell->SetCurrentPoint(
				     wxPoint(m_currentPoint.x + m_signWidth,
					     m_currentPoint.y + (m_innerCell->GetCenterList() -
								 m_innerCell->GetHeightList() / 2)));
      else
        m_innerCell->SetCurrentPoint(
				     wxPoint(m_currentPoint.x + m_signWidth, m_currentPoint.y));

      m_height =
	wxMax(m_signHeight, m_innerCell->GetHeightList()) + Scale_Px(4);
      m_center = m_height / 2;
    }
  }
  Cell::Recalculate(fontsize);
}

void ParenCell::Draw(wxPoint point) {
  Cell::Draw(point);
  if (DrawThisCell(point)) {
    wxDC *dc = m_configuration->GetDC();
    wxPoint innerCellPos(point);

    SetFont(m_configuration->GetMathFontSize());

    switch (m_bigParenType) {
    case Configuration::ascii:
      innerCellPos.x += m_open->GetWidth();
      m_open->DrawList(point);
      m_close->DrawList(wxPoint(
				point.x + m_open->GetWidth() + m_innerCell->GetFullWidth(), point.y));
      break;
    case Configuration::assembled_unicode:
    case Configuration::assembled_unicode_fallbackfont:
    case Configuration::assembled_unicode_fallbackfont2: {
      innerCellPos.x += m_signWidth;
      // Center the contents of the parenthesis vertically.
      innerCellPos.y +=
	(m_innerCell->GetCenterList() - m_innerCell->GetHeightList() / 2);

      int top = point.y - m_center + Scale_Px(1);
      int bottom = top + m_signHeight - m_signBotHeight - Scale_Px(2);
      dc->DrawText(wxT(PAREN_OPEN_TOP_UNICODE), point.x, top);
      dc->DrawText(wxT(PAREN_CLOSE_TOP_UNICODE),
                   point.x + m_signWidth + m_innerCell->GetFullWidth(), top);
      dc->DrawText(wxT(PAREN_OPEN_BOTTOM_UNICODE), point.x, bottom);
      dc->DrawText(wxT(PAREN_CLOSE_BOTTOM_UNICODE),
                   point.x + m_signWidth + m_innerCell->GetFullWidth(), bottom);

      for (int i = 0; i < m_numberOfExtensions; i++) {
        dc->DrawText(wxT(PAREN_OPEN_EXTEND_UNICODE), point.x,
                     top + m_signTopHeight + i * m_extendHeight);
        dc->DrawText(wxT(PAREN_CLOSE_EXTEND_UNICODE),
                     point.x + m_signWidth + m_innerCell->GetFullWidth(),
                     top + m_signTopHeight + i * m_extendHeight);
      }
    } break;
    default: {
      wxDC *adc = m_configuration->GetAntialiassingDC();
      innerCellPos.y +=
	(m_innerCell->GetCenterList() - m_innerCell->GetHeightList() / 2);
      SetPen(1.0);
      SetBrush();

      int signWidth = m_signWidth - Scale_Px(2);
      innerCellPos.x = point.x + m_signWidth;

      // Left bracket
      const wxPoint pointsL[10] = {
	{point.x + Scale_Px(1) + signWidth, point.y - m_center + Scale_Px(4)},
	{point.x + Scale_Px(1) + 3 * signWidth / 4,
	 point.y - m_center + 3 * signWidth / 4 + Scale_Px(4)},
	{point.x + Scale_Px(1), point.y},
	{point.x + Scale_Px(1) + 3 * signWidth / 4,
	 point.y + m_center - 3 * signWidth / 4 - Scale_Px(4)},
	{point.x + Scale_Px(1) + signWidth, point.y + m_center - Scale_Px(4)},
	// Appending the last point twice should allow for an abrupt 180° turn
	{point.x + Scale_Px(1) + signWidth, point.y + m_center - Scale_Px(4)},
	{point.x + Scale_Px(1) + 3 * signWidth / 4,
	 point.y + m_center - 3 * signWidth / 4 - Scale_Px(4)},
	// The middle point of the 2nd run of the parenthesis is at a
	// different place making the parenthesis wider here
	{point.x + Scale_Px(2), point.y},
	{point.x + Scale_Px(1) + 3 * signWidth / 4,
	 point.y - m_center + 3 * signWidth / 4 + Scale_Px(4)},
	{point.x + Scale_Px(1) + signWidth,
	 point.y - m_center + Scale_Px(4)}};
      adc->DrawSpline(10, pointsL);

      // Right bracket
      const wxPoint pointsR[10] = {
	{point.x + m_width - Scale_Px(1) - signWidth,
	 point.y - m_center + Scale_Px(4)},
	{point.x + m_width - Scale_Px(1) - signWidth / 2,
	 point.y - m_center + signWidth / 2 + Scale_Px(4)},
	{point.x + m_width - Scale_Px(1), point.y},
	{point.x + m_width - Scale_Px(1) - signWidth / 2,
	 point.y + m_center - signWidth / 2 - Scale_Px(4)},
	{point.x + m_width - Scale_Px(1) - signWidth,
	 point.y + m_center - Scale_Px(4)},
	{point.x + m_width - Scale_Px(1) - signWidth,
	 point.y + m_center - Scale_Px(4)},
	{point.x + m_width - Scale_Px(1) - signWidth / 2,
	 point.y + m_center - signWidth / 2 - Scale_Px(4)},
	{point.x + m_width - Scale_Px(2), point.y},
	{point.x + m_width - Scale_Px(1) - signWidth / 2,
	 point.y - m_center + signWidth / 2 + Scale_Px(4)},
	{point.x + m_width - Scale_Px(1) - signWidth,
	 point.y - m_center + Scale_Px(4)}};
      adc->DrawSpline(10, pointsR);
    } break;
    }

    if (!IsBrokenIntoLines())
      m_innerCell->DrawList(innerCellPos);
  }
}

wxString ParenCell::ToString() const {
  wxString s;
  if (!m_innerCell)
    return "()";

  if (!IsBrokenIntoLines()) {
    if (m_print)
      s = wxT("(") + m_innerCell->ListToString() + wxT(")");
    else
      s = m_innerCell->ListToString();
  }
  return s;
}

wxString ParenCell::ToMatlab() const {
  wxString s;
  if (!IsBrokenIntoLines()) {
    if (m_print)
      s = wxT("(") + m_innerCell->ListToMatlab() + wxT(")");
    else
      s = m_innerCell->ListToMatlab();
  }
  return s;
}

wxString ParenCell::ToTeX() const {
  wxString s;
  if (!IsBrokenIntoLines()) {
    wxString innerCell = m_innerCell->ListToTeX();

    // Let's see if the cell contains anything potentially higher than a normal
    // character.
    bool needsLeftRight = false;
    for (size_t i = 0; i < innerCell.Length(); i++)
      if (!wxIsalnum(innerCell[i])) {
        needsLeftRight = true;
        break;
      }

    if (m_print) {
      if (needsLeftRight)
        s = wxT("\\left( ") + m_innerCell->ListToTeX() + wxT("\\right) ");
      else
        s = wxT("(") + m_innerCell->ListToTeX() + wxT(")");
    } else
      s = m_innerCell->ListToTeX();
  }
  return s;
}

wxString ParenCell::ToOMML() const {
  return wxT("<m:d><m:dPr m:begChr=\"") + XMLescape(m_open->ToString()) +
    wxT("\" m:endChr=\"") + XMLescape(m_close->ToString()) +
    wxT("\" m:grow=\"1\"></m:dPr><m:e>") + m_innerCell->ListToOMML() +
    wxT("</m:e></m:d>");
}

wxString ParenCell::ToMathML() const {
  if (!m_print)
    return m_innerCell->ListToMathML();

  wxString open = m_open->ToString();
  wxString close = m_close->ToString();
  return (wxT("<mrow><mo>") + XMLescape(open) + wxT("</mo>") +
          m_innerCell->ListToMathML() + wxT("<mo>") + XMLescape(close) +
          wxT("</mo></mrow>\n"));
}

wxString ParenCell::ToXML() const {
  //  if(IsBrokenIntoLines())
  //    return wxEmptyString;
  wxString s = m_innerCell->ListToXML();
  wxString flags;
  if (HasHardLineBreak())
    flags += wxT(" breakline=\"true\"");
  return ((m_print) ? _T("<r><p") + flags + wxT(">") + s + _T("</p></r>") : s);
}

bool ParenCell::BreakUp() {
  if (IsBrokenIntoLines())
    return false;

  Cell::BreakUpAndMark();
  m_open->SetNextToDraw(m_innerCell);
  m_innerCell->last()->SetNextToDraw(m_close);
  m_close->SetNextToDraw(m_nextToDraw);
  m_nextToDraw = m_open;

  ResetCellListSizes();
  m_height = 0;
  m_center = 0;
  return true;
}

void ParenCell::SetNextToDraw(Cell *next) {
  if (IsBrokenIntoLines())
    m_close->SetNextToDraw(next);
  else
    m_nextToDraw = next;
}
