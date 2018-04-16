#include <gtest/gtest.h>
#include <gtirb/Module.hpp>
#include <gtirb/ModuleSummary.hpp>
#include <gtirb/NodeStructureError.hpp>
#include <memory>

TEST(Unit_ModuleSummary, ctor_0)
{
    EXPECT_NO_THROW(gtirb::ModuleSummary());
}

TEST(Unit_ModuleSummary, setName)
{
    const std::string name{"foo"};
    auto m = std::make_unique<gtirb::ModuleSummary>();

    EXPECT_NO_THROW(m->setName(name));
    EXPECT_EQ(name, m->getName());
}

TEST(Unit_ModuleSummary, getName)
{
    auto m = std::make_unique<gtirb::ModuleSummary>();
    EXPECT_NO_THROW(m->getName());
    EXPECT_TRUE(m->getName().empty());
}

TEST(Unit_ModuleSummary, setDecodeMode)
{
    const uint64_t decodeMode{0x10101010};
    auto m = std::make_unique<gtirb::ModuleSummary>();

    EXPECT_NO_THROW(m->setDecodeMode(decodeMode));
    EXPECT_EQ(decodeMode, m->getDecodeMode());
}

TEST(Unit_ModuleSummary, getDecodeMode)
{
    auto m = std::make_unique<gtirb::ModuleSummary>();
    EXPECT_NO_THROW(m->getDecodeMode());
    EXPECT_EQ(uint64_t{0}, m->getDecodeMode());
}

TEST(Unit_ModuleSummary, validParent)
{
    auto module = std::make_unique<gtirb::Module>();
    auto moduleSummary = std::make_unique<gtirb::ModuleSummary>();
    EXPECT_TRUE(moduleSummary->getIsValidParent(module.get()));
    EXPECT_TRUE(module->push_back(std::move(moduleSummary)));
}

TEST(Unit_ModuleSummary, validParent_noException)
{
    auto module = std::make_unique<gtirb::Module>();
    auto moduleSummary = std::make_unique<gtirb::ModuleSummary>();
    EXPECT_TRUE(moduleSummary->getIsValidParent(module.get()));
    EXPECT_NO_THROW(module->push_back(std::move(moduleSummary)));
}

TEST(Unit_ModuleSummary, invalidParent)
{
    auto notAModule = std::make_unique<gtirb::Node>();
    auto moduleSummary = std::make_unique<gtirb::ModuleSummary>();

    EXPECT_FALSE(moduleSummary->getIsValidParent(notAModule.get()));
    EXPECT_THROW(notAModule->push_back(std::move(moduleSummary)), gtirb::NodeStructureError);
}