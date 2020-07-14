// -*- mode: c++; c-file-style: "linux"; c-basic-offset: 2; indent-tabs-mode: nil -*-
//
//  Copyright (C) 2004-2015 Andrej Vodopivec <andrej.vodopivec@gmail.com>
//            (C) 2014-2016 Gunter Königsmann <wxMaxima@physikbuch.de>
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

#ifndef SUBCELL_H
#define SUBCELL_H

#include "Cell.h"

class SubCell final : public Cell
{
public:
  SubCell(GroupCell *parent, Configuration **config);
  SubCell(const SubCell &cell);
  Cell *Copy() const override { return new SubCell(*this); }

  InnerCellIterator InnerBegin() const override { return InnerCellIterator(&m_baseCell); }
  InnerCellIterator InnerEnd() const override { return ++InnerCellIterator(&m_indexCell); }

  void SetBase(Cell *base);

  void SetIndex(Cell *index);

  void RecalculateHeight(AFontSize fontsize) override;
  void RecalculateWidths(AFontSize fontsize) override;

  void Draw(wxPoint point) override;

  wxString ToMathML() const override;
  wxString ToMatlab() const override;
  wxString ToOMML() const override;
  wxString ToString() const override;
  wxString ToTeX() const override;
  wxString ToXML() const override;

  void SetAltCopyText(const wxString &text) { m_altCopyText = text; }

  void SetNextToDraw(Cell *next) override { m_nextToDraw = next; }
  Cell *GetNextToDraw() const override { return m_nextToDraw; }
  
private:
  //! Text that should end up on the clipboard if this cell is copied as text.
  wxString m_altCopyText;

  CellPtr<Cell> m_nextToDraw;

  // The pointers below point to inner cells and must be kept contiguous.
  // ** All pointers must be the same: either Cell * or std::unique_ptr<Cell>.
  // ** NO OTHER TYPES are allowed.
  std::unique_ptr<Cell> m_baseCell;
  std::unique_ptr<Cell> m_indexCell;
  // The pointers above point to inner cells and must be kept contiguous.
};

#endif // SUBCELL_H
