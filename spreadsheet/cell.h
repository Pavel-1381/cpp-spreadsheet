#pragma once

#include "common.h"
#include "formula.h"

#include <functional>
#include <unordered_set>

class Sheet;

std::ostream& operator<<(std::ostream& output, const CellInterface::Value& value);

class Cell : public CellInterface {
public:
    Cell(Sheet& sheet);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;

    std::unique_ptr<Impl> impl_;

    // проверить на цикличные зависимости
    bool IsCircularDependency(const Impl& newImpl) const;

    // обновить списки ячеек
    void UpdateRefs();

    // сбросить кэш
    void InvalidateCache(bool force = false);

    // ссылка на таблицу
    Sheet& sheet_;
    // множество ячеек, которые зависят от нас
    std::unordered_set<Cell*> dependentRefs_;
    // множество ячеек, от которых мы зависим
    std::unordered_set<Cell*> linkedRefs_;

};
