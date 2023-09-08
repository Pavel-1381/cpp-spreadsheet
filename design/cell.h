#pragma once

#include "common.h"
#include "formula.h"
#include <unordered_set>

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
	
	// множество ячеек, которые зависят от нас
    std::unordered_set<std::unique_ptr<Cell>> dependentRefs_;
	// множество ячеек, от которых мы зависим
    std::unordered_set<std::unique_ptr<Cell>> linkedRefs_;
	// ссылка на таблицу
    Sheet& sheet_;
	// проверить на цикличные зависимости
    bool IsCircularDependency(const Impl& impl_) const;
	// обновить списки ячеек
    void UpdateRefs();
	// сбросить кэш
    void InvalidateCache();
};
