#include "cell.h"
#include "sheet.h"
#include <cassert>
#include <iostream>
#include <string>
#include <optional>
#include <stack>
using namespace std::literals;

std::ostream& operator<<(std::ostream& output, const CellInterface::Value& value) {
    std::visit([&](const auto& x) { output << x; }, value);
    return output;
}

class Cell::Impl {
public:
    Impl() = default;

    virtual ~Impl() = default;

    virtual Value GetValue() const = 0;

    virtual std::string GetText() const = 0;

    virtual std::vector<Position> GetReferencedCells() const {
        return {};
    }

    virtual FormulaInterface* GetFormula() {
        return nullptr;
    }

    virtual bool IsCacheValid() const {
        return true;
    }

    virtual void InvalidateCache() {}
};

class Cell::EmptyImpl : public Impl {
public:
    Value GetValue() const override {
        return ""s;
    }

    std::string GetText() const override {
        return ""s;
    }
};

class Cell::TextImpl : public Impl {
public:
    explicit TextImpl(std::string str) : text_(std::move(str)) {
        if (text_.empty()) {
            throw std::invalid_argument("Empty string for TextCell");
        }
    }

    Value GetValue() const override {
        if (text_[0] == ESCAPE_SIGN) {
            return text_.substr(1);
        }
        return text_;
    }

    std::string GetText() const override {
        return text_;
    }

private:
    std::string text_;
};

class Cell::FormulaImpl : public Impl {
public:
    explicit FormulaImpl(std::string str, const SheetInterface& sheet)
        : formula_(ParseFormula(std::move(str)))
        , sheet_(sheet) {}

    Value GetValue() const override {
        auto cell_interface_value = formula_->Evaluate(sheet_);
        return std::visit(
            [](const auto& formula_interface_value) {return Value(formula_interface_value); }
        , cell_interface_value);
    }

    std::string GetText() const override {
        return FORMULA_SIGN + formula_->GetExpression();
    }
    std::vector<Position> GetReferencedCells() const override {
        return formula_->GetReferencedCells();
    }

    FormulaInterface* GetFormula() override {
        return formula_.get();
    }

    bool IsCacheValid() const override {
        return cache_.has_value();
    }

    void InvalidateCache() override {
        cache_.reset();
    }

private:
    std::unique_ptr<FormulaInterface> formula_;
    const SheetInterface& sheet_;
    mutable std::optional<FormulaInterface::Value> cache_;
};

Cell::Cell(Sheet& sheet) :
    impl_(std::make_unique<EmptyImpl>()),
    sheet_(sheet) {}

Cell::~Cell() {}

void Cell::Set(std::string text) {

    if (text.empty()) {
        impl_ = std::make_unique<EmptyImpl>();
        return;
    }
    if (text[0] == FORMULA_SIGN && text.size() > 1) {
        auto impl = std::make_unique<FormulaImpl>(text.substr(1), sheet_);
        if (IsCircularDependency(*impl)) {
            throw CircularDependencyException("Circular dependency");
        }
        impl_ = std::move(impl);
        auto referenced_cells = impl_->GetReferencedCells();
        for (auto& cell : referenced_cells) {
            if (!sheet_.GetCell(cell)) {
                sheet_.SetCell(cell, {});
            }
        }
        UpdateRefs();
        InvalidateCache(true);
        return;
    }
    impl_ = std::make_unique<TextImpl>(text);


}

void Cell::Clear() {
    Set(""); 
}

Cell::Value Cell::GetValue() const {
    return impl_->GetValue();
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const
{
    return impl_->GetReferencedCells();
}

bool Cell::IsCircularDependency(const Impl& newImpl) const
{
    if (newImpl.GetReferencedCells().empty()) {
        return false;
    }

    std::unordered_set<const Cell*> referenced;
    for (const auto& pos : newImpl.GetReferencedCells()) {
        referenced.insert(sheet_.GetCellAtPos(pos));
    }

    std::unordered_set<const Cell*> visited;
    std::stack<const Cell*> toVisit;
    toVisit.push(this);
    while (!toVisit.empty()) {
        const Cell* current = toVisit.top();
        toVisit.pop();
        visited.insert(current);
        if (referenced.find(current) != referenced.end()) {
            return true;
        }
        for (const Cell* incoming : current->dependentRefs_) {
            if (visited.find(incoming) == visited.end()) {
                toVisit.push(incoming);
            }
        }
    }

    return false;
}

void Cell::UpdateRefs()
{
    for (Cell* outgoing : linkedRefs_) {
        outgoing->dependentRefs_.erase(this);
    }
    linkedRefs_.clear();
    for (const auto& pos : impl_->GetReferencedCells()) {
        Cell* outgoing = sheet_.GetCellAtPos(pos);
        if (!outgoing) {
            sheet_.SetCell(pos, "");
            outgoing = sheet_.GetCellAtPos(pos);
        }
        linkedRefs_.insert(outgoing);
        outgoing->dependentRefs_.insert(this);
    }
}

void Cell::InvalidateCache(bool force) {
    if (impl_->IsCacheValid() || force) {
        impl_->InvalidateCache();
        for (Cell* incoming : dependentRefs_) {
            incoming->InvalidateCache();
        }
    }
}





