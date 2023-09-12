#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {
    for (auto& row : cells_) {
        for (auto& cell : row) {
            if (cell) {
                cell->Clear();
            }
        }
    }
}

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException(
            "Invalid position passed to Sheet::SetCell()");
    }
    auto s = Size{ pos.row + 1, pos.col + 1 };
    if (cells_.size() < (size_t)s.rows) {
        cells_.resize(s.rows);
    }
    if (cells_[pos.row].size() < (size_t)s.cols) {
        cells_[pos.row].resize(s.cols);
    }
    if (size_cached_.rows < s.rows) {
        size_cached_.rows = s.rows;
    }
    if (size_cached_.cols < s.cols) {
        size_cached_.cols = s.cols;
    }
    auto& cell = cells_[pos.row][pos.col];
    if (!cell) {
        cell = std::make_unique<Cell>(*this);
    }
    cell->Set(std::move(text));
}
const CellInterface* Sheet::GetCell(Position pos) const {
    return GetCellAtPos(pos);
}
CellInterface* Sheet::GetCell(Position pos) {
    return GetCellAtPos(pos);
}
const Cell* Sheet::GetCellAtPos(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException(
            "Invalid position passed to Sheet::GetCell()");
    }
    if ((size_t)pos.row >= size(cells_) || (size_t)pos.col >= size(cells_[pos.row])) {
        return nullptr;
    }
    return cells_[pos.row][pos.col].get();
}

Cell* Sheet::GetCellAtPos(Position pos) {
    return const_cast<Cell*>(
        static_cast<const Sheet&>(*this).GetCellAtPos(pos));
}
void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Wrong cell coordinates");
    }
    if ((size_t)pos.row < cells_.size() && (size_t)pos.col < cells_[pos.row].size()) { //проверим наличие ячейки
        cells_[pos.row][pos.col].release();
        //удалим все пустые ячейки в строке от текущей очищенной влево
        size_t i = pos.col;
        while (cells_[pos.row].size() > 0 && !cells_[pos.row][i]) {
            cells_[pos.row].pop_back();
            --i;
        }
        i = cells_.size() > 0 ? cells_.size() - 1 : 0;
        while (cells_.size() > 0 && cells_[i].size() == 0) {
            cells_.pop_back();
            --i;
        }
        size_cached_.rows = cells_.size();
        size_t max = 0;
        for (size_t i = 0; i < cells_.size(); ++i) {
            if (cells_[i].size() > max) {
                max = cells_[i].size();
            }
        }
        size_cached_.cols = max;
    }

}

Size Sheet::GetPrintableSize() const {
    return size_cached_;
}
void Sheet::PrintValues(std::ostream& output) const {
    for (const auto& r : cells_) {
        bool first = true;
        for (const auto& c : r) {
            if (!first) {
                output << '\t';
            }
            first = false;
            if (c) {
                output << c->GetValue();
            }
        }
        for (size_t i = 1; i < (size_cached_.cols - r.size()); ++i) {
            output << '\t';
        }
        output << '\n';
    }
}
void Sheet::PrintTexts(std::ostream& output) const {
    for (const auto& r : cells_) {
        bool first = true;
        for (const auto& c : r) {
            if (!first) {
                output << '\t';
            }
            first = false;
            if (c) {
                output << c->GetText();
            }
        }
        for (size_t i = 1; i < (size_cached_.cols - r.size()); ++i) {
            output << '\t';
        }
        output << '\n';
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}
