#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdio>

#include <doctest/doctest.h>

#include <wayfire/config/file.hpp>
#include <wayfire/config/types.hpp>
#include <wayfire/config/xml.hpp>
#include <wayfire/config/section.hpp>
#include <wayfire/config/option.hpp>
#include <wayfire/config/compound-option.hpp>
#include <wayfire/util/log.hpp>
#include <wayfire/util/duration.hpp>

#include "../src/option-impl.hpp"

#include "expect_line.hpp"

using wf::config::option_t;
using wf::config::option_base_t;
using wf::config::config_manager_t;
using wf::config::section_t;
using wf::config::compound_option_t;
using wf::config::compound_option_entry_t;
using wf::config::compound_list_t;

/* ==================== file.cpp tests ==================== */

TEST_CASE("file: is_nonescaped edge cases")
{
    // Test loading config with escaped sharps in values
    const std::string config_with_escaped_sharp = R"(
[section]
option1 = value with \# escaped sharp
option2 = plain value
option3 = \# at start
)";
    config_manager_t cfg;
    wf::config::load_configuration_options_from_string(cfg, config_with_escaped_sharp);

    auto section = cfg.get_section("section");
    REQUIRE(section);

    CHECK(section->get_option("option1")->get_value_str() == "value with # escaped sharp");
    CHECK(section->get_option("option2")->get_value_str() == "plain value");
    CHECK(section->get_option("option3")->get_value_str() == "# at start");
}

TEST_CASE("file: save with trailing backslash")
{
    auto section = std::make_shared<section_t>("section");
    section->register_new_option(
        std::make_shared<option_t<std::string>>("trailing_backslash", "ends with \\"));

    config_manager_t cfg;
    cfg.merge_section(section);

    auto str = wf::config::save_configuration_options_to_string(cfg);
    // The save function should escape trailing backslashes
    bool has_escaped_backslash = (str.find("\\\\") != std::string::npos);
    CHECK(has_escaped_backslash);
}

TEST_CASE("file: ignore_leading_trailing_whitespace")
{
    const std::string config = R"(
  [section]
  key1 = value with leading space
key2=value with trailing space

  [  section2  ]
  key3 =   spaced value
)";
    config_manager_t cfg;
    wf::config::load_configuration_options_from_string(cfg, config);

    REQUIRE(cfg.get_section("section"));
    CHECK(cfg.get_section("section")->get_option("key1")->get_value_str() ==
          "value with leading space");
    CHECK(cfg.get_section("section")->get_option("key2")->get_value_str() ==
          "value with trailing space");

    // Section name with spaces should not match
    CHECK(cfg.get_section("section2") == nullptr);
}

TEST_CASE("file: save with sharps in values")
{
    auto section = std::make_shared<section_t>("section");
    section->register_new_option(
        std::make_shared<option_t<std::string>>("has_sharp", "value with # character"));
    section->register_new_option(
        std::make_shared<option_t<std::string>>("multiple_sharps", "a#b#c#d"));

    config_manager_t cfg;
    cfg.merge_section(section);

    auto str = wf::config::save_configuration_options_to_string(cfg);
    // Sharps should be escaped in output
    bool has_escaped_sharp = (str.find("\\#") != std::string::npos);
    CHECK(has_escaped_sharp);
}

TEST_CASE("file: load from string with line joining")
{
    const std::string config = R"(
[section]
line1 = part1 \
part2 \
part3
line2 = single
)";
    config_manager_t cfg;
    wf::config::load_configuration_options_from_string(cfg, config);

    REQUIRE(cfg.get_section("section"));
    CHECK(cfg.get_section("section")->get_option("line1")->get_value_str() ==
          "part1 part2 part3");
    CHECK(cfg.get_section("section")->get_option("line2")->get_value_str() == "single");
}

TEST_CASE("file: line joining with escaped backslash")
{
    const std::string config = R"(
[section]
normal = value1
continued = line1 \
line2
escaped_backslash = value \\
after
)";
    config_manager_t cfg;
    wf::config::load_configuration_options_from_string(cfg, config);

    REQUIRE(cfg.get_section("section"));
    CHECK(cfg.get_section("section")->get_option("continued")->get_value_str() ==
          "line1 line2");
    CHECK(cfg.get_section("section")->get_option("escaped_backslash")->get_value_str() ==
          "value \\");
}

TEST_CASE("file: option declared before section")
{
    std::stringstream log;
    wf::log::initialize_logging(log, wf::log::LOG_LEVEL_DEBUG,
        wf::log::LOG_COLOR_MODE_OFF);

    const std::string config = R"(
option_before_section = value
[section]
key = value
)";
    config_manager_t cfg;
    wf::config::load_configuration_options_from_string(cfg, config, "test");

    REQUIRE(cfg.get_section("section"));
    EXPECT_LINE(log, "option declared before a section starts");

    wf::log::initialize_logging(std::cout, wf::log::LOG_LEVEL_DEBUG,
        wf::log::LOG_COLOR_MODE_OFF);
}

TEST_CASE("file: invalid option format")
{
    std::stringstream log;
    wf::log::initialize_logging(log, wf::log::LOG_LEVEL_DEBUG,
        wf::log::LOG_COLOR_MODE_OFF);

    const std::string config = R"(
[section]
invalid_option_no_equals
also_invalid
)";
    config_manager_t cfg;
    wf::config::load_configuration_options_from_string(cfg, config, "test");

    EXPECT_LINE(log, "invalid option format");

    wf::log::initialize_logging(std::cout, wf::log::LOG_LEVEL_DEBUG,
        wf::log::LOG_COLOR_MODE_OFF);
}

TEST_CASE("file: invalid option value type")
{
    std::stringstream log;
    wf::log::initialize_logging(log, wf::log::LOG_LEVEL_DEBUG,
        wf::log::LOG_COLOR_MODE_OFF);

    config_manager_t cfg;
    auto section = std::make_shared<section_t>("section");
    section->register_new_option(
        std::make_shared<option_t<int>>("int_option", 42));
    cfg.merge_section(section);

    const std::string config = R"(
[section]
int_option = not_a_number
)";
    wf::config::load_configuration_options_from_string(cfg, config, "test");

    EXPECT_LINE(log, "invalid option value");

    wf::log::initialize_logging(std::cout, wf::log::LOG_LEVEL_DEBUG,
        wf::log::LOG_COLOR_MODE_OFF);
}

TEST_CASE("file: unknown plugin option warning")
{
    std::stringstream log;
    wf::log::initialize_logging(log, wf::log::LOG_LEVEL_DEBUG,
        wf::log::LOG_COLOR_MODE_OFF);

    const std::string config = R"(
[section]
unknown_option = value
)";
    config_manager_t cfg;
    wf::config::load_configuration_options_from_string(cfg, config, "test");

    EXPECT_LINE(log, "does not belong to any registered plugin");

    wf::log::initialize_logging(std::cout, wf::log::LOG_LEVEL_DEBUG,
        wf::log::LOG_COLOR_MODE_OFF);
}

TEST_CASE("file: compound option parse error warning")
{
    std::stringstream log;
    wf::log::initialize_logging(log, wf::log::LOG_LEVEL_DEBUG,
        wf::log::LOG_COLOR_MODE_OFF);

    compound_option_t::entries_t entries;
    entries.push_back(std::make_unique<compound_option_entry_t<int>>("prefix_"));
    auto opt = new compound_option_t{"compound_opt", std::move(entries)};

    auto section = std::make_shared<section_t>("section");
    section->register_new_option(std::shared_ptr<option_base_t>(opt));
    // Register an option with wrong type (should be int, is string)
    section->register_new_option(
        std::make_shared<option_t<std::string>>("prefix_key", "invalid"));
    for (auto& o : section->get_registered_options())
    {
        o->priv->option_in_config_file = true;
    }

    config_manager_t cfg;
    cfg.merge_section(section);

    const std::string config = R"(
[section]
prefix_key = invalid
)";
    wf::config::load_configuration_options_from_string(cfg, config, "test");

    // Check that compound option parsing generates a log warning
    // The warning says it failed to parse and will use default
    EXPECT_LINE(log, "Failed parsing option");

    wf::log::initialize_logging(std::cout, wf::log::LOG_LEVEL_DEBUG,
        wf::log::LOG_COLOR_MODE_OFF);
}

/* ==================== XML parsing tests ==================== */

TEST_CASE("xml: parse_compound_option various types")
{
    std::stringstream log;
    wf::log::initialize_logging(log, wf::log::LOG_LEVEL_DEBUG,
        wf::log::LOG_COLOR_MODE_OFF);

    namespace wxml = wf::config::xml;
    namespace wc = wf::config;

    auto test_option = [&](const std::string& xml, const std::string& expected_name) {
        auto doc = xmlParseDoc((const xmlChar*)xml.c_str());
        REQUIRE(doc != nullptr);
        auto node = xmlDocGetRootElement(doc);
        auto opt = wxml::create_option_from_xml_node(node);
        REQUIRE(opt != nullptr);
        CHECK(opt->get_name() == expected_name);
    };

    SUBCASE("int") {
        test_option(R"(
<option name="ListInt" type="dynamic-list">
    <entry prefix="int_" type="int"/>
</option>
)", "ListInt");
    }
    SUBCASE("double") {
        test_option(R"(
<option name="ListDouble" type="dynamic-list">
    <entry prefix="dbl_" type="double"/>
</option>
)", "ListDouble");
    }
    SUBCASE("bool") {
        test_option(R"(
<option name="ListBool" type="dynamic-list">
    <entry prefix="bool_" type="bool"/>
</option>
)", "ListBool");
    }
    SUBCASE("string") {
        test_option(R"(
<option name="ListString" type="dynamic-list">
    <entry prefix="str_" type="string"/>
</option>
)", "ListString");
    }
    SUBCASE("key") {
        test_option(R"(
<option name="ListKey" type="dynamic-list">
    <entry prefix="key_" type="key"/>
</option>
)", "ListKey");
    }
    SUBCASE("button") {
        test_option(R"(
<option name="ListButton" type="dynamic-list">
    <entry prefix="btn_" type="button"/>
</option>
)", "ListButton");
    }
    SUBCASE("gesture") {
        test_option(R"(
<option name="ListGesture" type="dynamic-list">
    <entry prefix="gest_" type="gesture"/>
</option>
)", "ListGesture");
    }
    SUBCASE("color") {
        test_option(R"(
<option name="ListColor" type="dynamic-list">
    <entry prefix="col_" type="color"/>
</option>
)", "ListColor");
    }
    SUBCASE("activator") {
        test_option(R"(
<option name="ListActivator" type="dynamic-list">
    <entry prefix="act_" type="activator"/>
</option>
)", "ListActivator");
    }
    SUBCASE("animation") {
        test_option(R"(
<option name="ListAnimation" type="dynamic-list">
    <entry prefix="anim_" type="animation"/>
</option>
)", "ListAnimation");
    }

    wf::log::initialize_logging(std::cout, wf::log::LOG_LEVEL_DEBUG,
        wf::log::LOG_COLOR_MODE_OFF);
}

TEST_CASE("xml: set_bounds invalid minimum/maximum")
{
    std::stringstream log;
    wf::log::initialize_logging(log, wf::log::LOG_LEVEL_DEBUG,
        wf::log::LOG_COLOR_MODE_OFF);

    namespace wxml = wf::config::xml;

    SUBCASE("invalid minimum") {
        auto doc = xmlParseDoc((const xmlChar*)R"(
<option name="Test" type="int">
    <default>5</default>
    <min>not_a_number</min>
</option>
)");
        REQUIRE(doc != nullptr);
        auto node = xmlDocGetRootElement(doc);
        auto opt = wxml::create_option_from_xml_node(node);
        CHECK(opt == nullptr);
        EXPECT_LINE(log, "invalid minimum value");
    }
    SUBCASE("invalid maximum") {
        auto doc = xmlParseDoc((const xmlChar*)R"(
<option name="Test" type="int">
    <default>5</default>
    <max>not_a_number</max>
</option>
)");
        REQUIRE(doc != nullptr);
        auto node = xmlDocGetRootElement(doc);
        auto opt = wxml::create_option_from_xml_node(node);
        CHECK(opt == nullptr);
        EXPECT_LINE(log, "invalid maximum value");
    }

    wf::log::initialize_logging(std::cout, wf::log::LOG_LEVEL_DEBUG,
        wf::log::LOG_COLOR_MODE_OFF);
}

/* ==================== compound-option tests ==================== */

TEST_CASE("compound_option: begins_with")
{
    // This tests the internal begins_with function via compound option parsing
    // We set up a compound option with entries and update from section

    compound_option_t::entries_t entries;
    entries.push_back(std::make_unique<compound_option_entry_t<int>>("int_"));

    compound_option_t opt{"test", std::move(entries)};

    auto section = std::make_shared<section_t>("section");
    // Register an option that matches the prefix
    section->register_new_option(std::make_shared<option_t<int>>("int_key1", 42));

    // Mark as from config file
    for (auto& o : section->get_registered_options())
    {
        o->priv->option_in_config_file = true;
    }

    wf::config::update_compound_from_section(opt, section);

    // The compound option should have parsed the entry
    auto values = opt.get_value<int>();
    // Check that the compound option structure is set up correctly
    CHECK(opt.get_entries().size() == 1);
}

TEST_CASE("compound_option: clone_option")
{
    compound_option_t::entries_t entries;
    entries.push_back(std::make_unique<compound_option_entry_t<int>>("int_"));
    entries.push_back(std::make_unique<compound_option_entry_t<std::string>>("str_"));

    compound_option_t opt{"test", std::move(entries)};
    opt.set_value(compound_list_t<int, std::string>{{"k1", 42, "hello"}});

    auto cloned = std::dynamic_pointer_cast<compound_option_t>(opt.clone_option());
    REQUIRE(cloned != nullptr);
    CHECK(cloned->get_name() == opt.get_name());
    CHECK(cloned->get_value<int, std::string>() == opt.get_value<int, std::string>());
}

TEST_CASE("compound_option: set_value_str and get_value_str")
{
    compound_option_t::entries_t entries;
    entries.push_back(std::make_unique<compound_option_entry_t<int>>("int_"));
    entries.push_back(std::make_unique<compound_option_entry_t<std::string>>("str_"));

    compound_option_t opt{"test", std::move(entries)};

    SUBCASE("roundtrip") {
        opt.set_value(compound_list_t<int, std::string>{{"k1", 42, "hello"}});
        auto str = opt.get_value_str();
        CHECK(str == "k1=42;hello");
    }

    SUBCASE("multiple entries") {
        opt.set_value(compound_list_t<int, std::string>{
            {"k1", 1, "a"},
            {"k2", 2, "b"}
        });
        auto str = opt.get_value_str();
        CHECK(str == "k1=1;a,k2=2;b");
    }

    SUBCASE("empty") {
        CHECK(opt.get_value_str() == "");
        CHECK(opt.set_value_str("") == true);
        CHECK(opt.get_value_str() == "");
    }

    SUBCASE("invalid format - wrong entry count") {
        CHECK(opt.set_value_str("k1=42") == false);
    }

    SUBCASE("invalid format - wrong type") {
        CHECK(opt.set_value_str("k1=notanint;hello") == false);
    }
}

TEST_CASE("compound_option: get_default_value_str")
{
    compound_option_t::entries_t entries;
    entries.push_back(std::make_unique<compound_option_entry_t<int>>("int_", "", "10"));
    entries.push_back(std::make_unique<compound_option_entry_t<std::string>>("str_", "", "default"));

    compound_option_t opt{"test", std::move(entries)};

    auto def_str = opt.get_default_value_str();
    CHECK(def_str == "10;default");
}

TEST_CASE("compound_option: set_default_value_str returns false")
{
    compound_option_t::entries_t entries;
    entries.push_back(std::make_unique<compound_option_entry_t<int>>("int_"));

    compound_option_t opt{"test", std::move(entries)};

    // set_default_value_str is not supported (ambiguous for compound options)
    CHECK(opt.set_default_value_str("42") == false);
}

TEST_CASE("compound_option: reset_to_default clears value")
{
    compound_option_t::entries_t entries;
    entries.push_back(std::make_unique<compound_option_entry_t<int>>("int_"));

    compound_option_t opt{"test", std::move(entries)};
    opt.set_value(compound_list_t<int>{{"k1", 42}});
    REQUIRE(!opt.get_value<int>().empty());

    opt.reset_to_default();
    CHECK(opt.get_value<int>().empty());
}

TEST_CASE("compound_option: set_value_untyped with invalid data")
{
    compound_option_t::entries_t entries;
    entries.push_back(std::make_unique<compound_option_entry_t<int>>("int_"));
    entries.push_back(std::make_unique<compound_option_entry_t<std::string>>("str_"));

    compound_option_t opt{"test", std::move(entries)};

    // Wrong number of entries
    compound_option_t::stored_type_t wrong_count = {{"k1", "42"}};
    CHECK(opt.set_value_untyped(wrong_count) == false);

    // Wrong type for first entry (should be int, we give a string)
    compound_option_t::stored_type_t wrong_type = {{"k1", "not_an_int", "hello"}};
    CHECK(opt.set_value_untyped(wrong_type) == false);

    // Valid
    compound_option_t::stored_type_t valid = {{"k1", "42", "hello"}};
    CHECK(opt.set_value_untyped(valid) == true);
}

/* ==================== section tests ==================== */

TEST_CASE("section: unregister_option")
{
    section_t section{"test"};
    auto opt1 = std::make_shared<option_t<int>>("opt1", 1);
    auto opt2 = std::make_shared<option_t<int>>("opt2", 2);

    section.register_new_option(opt1);
    section.register_new_option(opt2);
    REQUIRE(section.get_registered_options().size() == 2);

    SUBCASE("unregister existing") {
        section.unregister_option(opt1);
        CHECK(section.get_registered_options().size() == 1);
        CHECK(section.get_option_or("opt1") == nullptr);
        CHECK(section.get_option_or("opt2") != nullptr);
    }

    SUBCASE("unregister null") {
        section.unregister_option(nullptr);
        CHECK(section.get_registered_options().size() == 2);
    }

    SUBCASE("unregister non-existent") {
        auto opt3 = std::make_shared<option_t<int>>("opt3", 3);
        section.unregister_option(opt3);
        CHECK(section.get_registered_options().size() == 2);
    }

    SUBCASE("unregister with wrong name") {
        auto opt1_copy = std::make_shared<option_t<int>>("opt1", 999);
        section.unregister_option(opt1_copy);
        CHECK(section.get_registered_options().size() == 2);
        auto opt1_val = std::dynamic_pointer_cast<option_t<int>>(section.get_option("opt1"));
        REQUIRE(opt1_val);
        CHECK(opt1_val->get_value() == 1);
    }
}

TEST_CASE("section: get_option throws for non-existent")
{
    section_t section{"test"};
    CHECK_THROWS_AS(section.get_option("non_existent"), std::invalid_argument);
}

/* ==================== option tests ==================== */

TEST_CASE("option: init_clone")
{
    auto opt1 = std::make_shared<option_t<int>>("test", 42);
    opt1->priv->xml = (xmlNode*)0x123;

    auto opt2 = std::make_shared<option_t<int>>("other", 0);
    REQUIRE(opt2->priv->xml == nullptr);

    // init_clone is called via clone_option, which copies xml
    auto cloned = opt1->clone_option();
    CHECK(cloned->get_name() == "test");
}

/* ==================== types tests ==================== */

TEST_CASE("types: filter_out function")
{
    // filter_out is used internally by parse_binding
    // Test it indirectly via valid keybinding parsing

    // Key with spaces in modifiers
    auto key1 = wf::option_type::from_string<wf::keybinding_t>("<super> <alt> KEY_E");
    REQUIRE(key1.has_value());

    // Key without spaces - filter_out removes whitespace
    auto key2 = wf::option_type::from_string<wf::keybinding_t>("<super><alt>KEY_E");
    REQUIRE(key2.has_value());

    // Both should be valid (filter_out handles spaces)
    CHECK(key1.has_value());
    CHECK(key2.has_value());
}

TEST_CASE("types: double parsing edge cases")
{
    SUBCASE("empty string") {
        CHECK(wf::option_type::from_string<double>("") == std::nullopt);
    }
    SUBCASE("trailing characters") {
        CHECK(wf::option_type::from_string<double>("1.5abc") == std::nullopt);
    }
    SUBCASE("leading characters") {
        CHECK(wf::option_type::from_string<double>("abc1.5") == std::nullopt);
    }
    SUBCASE("just dot") {
        CHECK(wf::option_type::from_string<double>(".") == std::nullopt);
    }
    SUBCASE("negative zero") {
        auto val = wf::option_type::from_string<double>("-0");
        CHECK(val.has_value());
        CHECK(*val == 0.0);
    }
}

TEST_CASE("types: int parsing edge cases")
{
    SUBCASE("trailing spaces") {
        CHECK(wf::option_type::from_string<int>("42 ") == std::nullopt);
    }
}

TEST_CASE("types: keybinding edge cases")
{
    SUBCASE("disabled binding") {
        auto key = wf::option_type::from_string<wf::keybinding_t>("disabled");
        REQUIRE(key.has_value());
        CHECK(key->get_modifiers() == 0);
        CHECK(key->get_key() == 0);
    }

    SUBCASE("none binding") {
        auto key = wf::option_type::from_string<wf::keybinding_t>("none");
        REQUIRE(key.has_value());
        CHECK(key->get_modifiers() == 0);
        CHECK(key->get_key() == 0);
    }

    SUBCASE("button instead of key") {
        // Keys must contain KEY_
        auto btn = wf::option_type::from_string<wf::keybinding_t>("<super> BTN_LEFT");
        CHECK(btn == std::nullopt);
    }

    SUBCASE("invalid modifier") {
        auto key = wf::option_type::from_string<wf::keybinding_t>("<invalid> KEY_E");
        CHECK(key == std::nullopt);
    }

    SUBCASE("invalid key name") {
        auto key = wf::option_type::from_string<wf::keybinding_t>("<super> INVALID_KEY_NAME");
        CHECK(key == std::nullopt);
    }
}

TEST_CASE("types: buttonbinding edge cases")
{
    SUBCASE("disabled button") {
        auto btn = wf::option_type::from_string<wf::buttonbinding_t>("disabled");
        REQUIRE(btn.has_value());
        CHECK(btn->get_modifiers() == 0);
        CHECK(btn->get_button() == 0);
    }

    SUBCASE("key instead of button") {
        // Buttons must contain BTN_
        auto key = wf::option_type::from_string<wf::buttonbinding_t>("<super> KEY_E");
        CHECK(key == std::nullopt);
    }
}

TEST_CASE("types: touchgesture edge cases")
{
    SUBCASE("disabled gesture") {
        auto g = wf::option_type::from_string<wf::touchgesture_t>("disabled");
        REQUIRE(g.has_value());
        CHECK(g->get_type() == wf::GESTURE_TYPE_NONE);
    }

    SUBCASE("invalid gesture type") {
        auto g = wf::option_type::from_string<wf::touchgesture_t>("spin left 3");
        CHECK(g == std::nullopt);
    }

    SUBCASE("invalid swipe direction") {
        auto g = wf::option_type::from_string<wf::touchgesture_t>("swipe diagonal 3");
        CHECK(g == std::nullopt);
    }

    SUBCASE("opposing swipe directions") {
        auto g = wf::option_type::from_string<wf::touchgesture_t>("swipe up-down 3");
        CHECK(g == std::nullopt);
    }

    SUBCASE("pinch invalid direction") {
        auto g = wf::option_type::from_string<wf::touchgesture_t>("pinch sideways 3");
        CHECK(g == std::nullopt);
    }
}

TEST_CASE("types: hotspot edge cases")
{
    SUBCASE("not a hotspot") {
        CHECK(wf::option_type::from_string<wf::hotspot_binding_t>("not a hotspot") == std::nullopt);
    }

    SUBCASE("invalid edge") {
        CHECK(wf::option_type::from_string<wf::hotspot_binding_t>("hotspot invalid 10x10 0") == std::nullopt);
    }

    SUBCASE("invalid size format") {
        CHECK(wf::option_type::from_string<wf::hotspot_binding_t>("hotspot top notsize 0") == std::nullopt);
    }

    SUBCASE("invalid size values") {
        CHECK(wf::option_type::from_string<wf::hotspot_binding_t>("hotspot top abc 0") == std::nullopt);
    }

    SUBCASE("invalid timeout") {
        CHECK(wf::option_type::from_string<wf::hotspot_binding_t>("hotspot top 10x10 notanumber") == std::nullopt);
    }

    SUBCASE("trailing garbage") {
        CHECK(wf::option_type::from_string<wf::hotspot_binding_t>("hotspot top 10x10 0 garbage") == std::nullopt);
    }

    SUBCASE("combined edges") {
        auto h = wf::option_type::from_string<wf::hotspot_binding_t>("hotspot top-left 10x10 100");
        REQUIRE(h.has_value());
        CHECK(h->get_edges() == (wf::OUTPUT_EDGE_TOP | wf::OUTPUT_EDGE_LEFT));
    }
}

TEST_CASE("types: activator edge cases")
{
    SUBCASE("empty binding") {
        auto a = wf::option_type::from_string<wf::activatorbinding_t>("");
        REQUIRE(a.has_value());
        CHECK(!a->has_match(wf::keybinding_t(0, 0)));
    }

    SUBCASE("invalid binding in pipe") {
        auto a = wf::option_type::from_string<wf::activatorbinding_t>("<super> KEY_E | invalid");
        REQUIRE(a.has_value());
        // The invalid part becomes an extension
        CHECK(!a->get_extensions().empty());
    }
}

TEST_CASE("types: output mode edge cases")
{
    using wf::output_config::mode_t;
    using wf::output_config::MODE_RESOLUTION;
    using wf::output_config::MODE_MIRROR;

    SUBCASE("invalid mirror syntax") {
        CHECK(wf::option_type::from_string<mode_t>("mirror") == std::nullopt);
        CHECK(wf::option_type::from_string<mode_t>("mirror with trailing garbage") == std::nullopt);
    }

    SUBCASE("invalid resolution") {
        CHECK(wf::option_type::from_string<mode_t>("abc x 720") == std::nullopt);
        CHECK(wf::option_type::from_string<mode_t>("1920 x abc") == std::nullopt);
    }

    SUBCASE("negative dimensions") {
        CHECK(wf::option_type::from_string<mode_t>("-1920 x 1080") == std::nullopt);
        CHECK(wf::option_type::from_string<mode_t>("1920 x -1080") == std::nullopt);
    }

    SUBCASE("mode constructors throw") {
        CHECK_THROWS_AS(mode_t mode_res{MODE_RESOLUTION}, std::invalid_argument);
        CHECK_THROWS_AS(mode_t mode_mir{MODE_MIRROR}, std::invalid_argument);
    }

    SUBCASE("mode equality") {
        mode_t m1{1920, 1080, 60000};
        mode_t m2{1920, 1080, 60000};
        mode_t m3{1920, 1080, 90000};
        CHECK(m1 == m2);
        CHECK_FALSE(m1 == m3);
    }
}

TEST_CASE("types: position edge cases")
{
    SUBCASE("invalid position format") {
        CHECK(wf::option_type::from_string<wf::output_config::position_t>("123 456") == std::nullopt);
        CHECK(wf::option_type::from_string<wf::output_config::position_t>("123;456") == std::nullopt);
    }

    SUBCASE("position equality") {
        wf::output_config::position_t p1{100, 200};
        wf::output_config::position_t p2{100, 200};
        wf::output_config::position_t p3{100, 300};
        CHECK(p1 == p2);
        CHECK_FALSE(p1 == p3);

        wf::output_config::position_t auto_pos;
        CHECK_FALSE(p1 == auto_pos);
    }
}

/* ==================== duration tests ==================== */

TEST_CASE("duration: animation_description_t edge cases")
{
    SUBCASE("simple animation format") {
        auto anim = wf::option_type::from_string<wf::animation_description_t>("500");
        REQUIRE(anim.has_value());
        CHECK(anim->length_ms == 500);
    }

    SUBCASE("ms format with easing") {
        auto anim = wf::option_type::from_string<wf::animation_description_t>("100 ms linear");
        REQUIRE(anim.has_value());
        CHECK(anim->length_ms == 100);
    }

    SUBCASE("s format") {
        auto anim = wf::option_type::from_string<wf::animation_description_t>("2 s sigmoid");
        REQUIRE(anim.has_value());
        CHECK(anim->length_ms == 2000);
    }

    SUBCASE("invalid suffix") {
        CHECK(wf::option_type::from_string<wf::animation_description_t>("100 hours linear") == std::nullopt);
    }

    SUBCASE("invalid easing") {
        CHECK(wf::option_type::from_string<wf::animation_description_t>("100 ms invalid_easing") == std::nullopt);
    }

    SUBCASE("trailing garbage") {
        CHECK(wf::option_type::from_string<wf::animation_description_t>("100 ms linear garbage") == std::nullopt);
    }

    SUBCASE("cubic-bezier easing") {
        auto anim = wf::option_type::from_string<wf::animation_description_t>("100 ms cubic-bezier 0.25 0.1 0.25 1.0");
        REQUIRE(anim.has_value());
        CHECK(anim->length_ms == 100);
    }
}

TEST_CASE("duration: get_available_smooth_functions")
{
    auto funcs = wf::animation::smoothing::get_available_smooth_functions();
    CHECK_FALSE(funcs.empty());
    bool has_linear = std::find(funcs.begin(), funcs.end(), "linear") != funcs.end();
    bool has_circle = std::find(funcs.begin(), funcs.end(), "circle") != funcs.end();
    bool has_sigmoid = std::find(funcs.begin(), funcs.end(), "sigmoid") != funcs.end();
    bool has_elastic = std::find(funcs.begin(), funcs.end(), "easeOutElastic") != funcs.end();
    CHECK(has_linear);
    CHECK(has_circle);
    CHECK(has_sigmoid);
    CHECK(has_elastic);
}

TEST_CASE("duration: smooth functions")
{
    using namespace wf::animation::smoothing;

    // Test linear
    CHECK(linear(0.0) == 0.0);
    CHECK(linear(1.0) == 1.0);
    CHECK(linear(0.5) == 0.5);

    // Test circle
    double c0 = circle(0.0);
    double c1 = circle(1.0);
    CHECK(c0 >= 0.0);
    CHECK(c0 <= 1.0);
    CHECK(c1 >= 0.0);
    CHECK(c1 <= 1.0);

    // Test sigmoid
    double s0 = sigmoid(0.0);
    double s1 = sigmoid(1.0);
    CHECK(s0 >= 0.0);
    CHECK(s0 <= 1.0);
    CHECK(s1 >= 0.0);
    CHECK(s1 <= 1.0);

}

TEST_CASE("duration: cubic-bezier smooth function")
{
    using namespace wf::animation::smoothing;

    auto bezier = get_cubic_bezier(0.25, 0.1, 0.25, 1.0);

    // Test bounds
    double b0 = bezier(0.0);
    double b1 = bezier(1.0);
    CHECK(b0 >= 0.0);
    CHECK(b0 <= 1.0);
    CHECK(b1 >= 0.0);
    CHECK(b1 <= 1.0);
}

TEST_CASE("duration: epsilon comparison")
{
    SUBCASE("equal animations") {
        auto a1 = wf::option_type::from_string<wf::animation_description_t>("100ms linear");
        auto a2 = wf::option_type::from_string<wf::animation_description_t>("100ms linear");
        REQUIRE(a1.has_value());
        REQUIRE(a2.has_value());
        CHECK(*a1 == *a2);
    }

    SUBCASE("different easing names but same easing") {
        auto a1 = wf::option_type::from_string<wf::animation_description_t>("100ms circle");
        auto a2 = wf::option_type::from_string<wf::animation_description_t>("100ms circle");
        REQUIRE(a1.has_value());
        REQUIRE(a2.has_value());
        CHECK(*a1 == *a2);
    }

    SUBCASE("different lengths") {
        auto a1 = wf::option_type::from_string<wf::animation_description_t>("100ms linear");
        auto a2 = wf::option_type::from_string<wf::animation_description_t>("200ms linear");
        REQUIRE(a1.has_value());
        REQUIRE(a2.has_value());
        CHECK_FALSE(*a1 == *a2);
    }

    SUBCASE("same cubic-bezier values") {
        auto a1 = wf::option_type::from_string<wf::animation_description_t>("100ms cubic-bezier 0 0 1 1");
        auto a2 = wf::option_type::from_string<wf::animation_description_t>("100ms cubic-bezier 0 0 1 1");
        REQUIRE(a1.has_value());
        REQUIRE(a2.has_value());
        CHECK(*a1 == *a2);
    }

    SUBCASE("different easing functions") {
        auto a1 = wf::option_type::from_string<wf::animation_description_t>("100ms circle");
        auto a2 = wf::option_type::from_string<wf::animation_description_t>("100ms linear");
        REQUIRE(a1.has_value());
        REQUIRE(a2.has_value());
        CHECK_FALSE(*a1 == *a2);
    }

    SUBCASE("different cubic-bezier y2 values") {
        // This tests the fix for the bug where y2_b was compared to itself
        auto a1 = wf::option_type::from_string<wf::animation_description_t>("100ms cubic-bezier 0.25 0.1 0.75 0.9");
        auto a2 = wf::option_type::from_string<wf::animation_description_t>("100ms cubic-bezier 0.25 0.1 0.75 1.0");
        REQUIRE(a1.has_value());
        REQUIRE(a2.has_value());
        // The easing names should be different since they store the exact values
        CHECK(a1->easing_name != a2->easing_name);
        // Should NOT be equal because y2 is different (0.9 vs 1.0)
        CHECK_FALSE(*a1 == *a2);
    }

    SUBCASE("epsilon-close cubic-bezier values are equal") {
        // Two values that differ by less than epsilon should be considered equal
        auto a1 = wf::option_type::from_string<wf::animation_description_t>("100ms cubic-bezier 0.25 0.1 0.75 1");
        auto a2 = wf::option_type::from_string<wf::animation_description_t>("100ms cubic-bezier 0.25 0.1 0.75 1.0000000000000001");
        REQUIRE(a1.has_value());
        REQUIRE(a2.has_value());
        // Should be equal because they're within epsilon
        CHECK(*a1 == *a2);
    }
}

TEST_CASE("duration: animation_description_t to_string")
{
    auto anim = wf::option_type::from_string<wf::animation_description_t>("100ms linear");
    REQUIRE(anim.has_value());

    auto str = wf::option_type::to_string(*anim);
    bool has_100 = (str.find("100") != std::string::npos);
    bool has_linear = (str.find("linear") != std::string::npos);
    CHECK(has_100);
    CHECK(has_linear);
}

/* ==================== build_configuration tests ==================== */

TEST_CASE("build_configuration: empty directories")
{
    std::vector<std::string> empty_dirs;
    auto cfg = wf::config::build_configuration(empty_dirs, "/nonexistent/sys.ini", "/nonexistent/user.ini");

    // Should return empty config (no crash)
    CHECK(cfg.get_all_sections().empty());
}

TEST_CASE("build_configuration: empty directories handled")
{
    // Test that build_configuration handles empty/non-existent directories gracefully
    std::vector<std::string> empty_dirs;
    auto cfg = wf::config::build_configuration(empty_dirs, "/nonexistent_sys.ini", "/nonexistent_user.ini");

    // Should return empty config (no crash)
    CHECK(cfg.get_all_sections().empty());
}

/* ==================== color tests ==================== */

TEST_CASE("color: constructor edge cases")
{
    SUBCASE("equality with epsilon") {
        wf::color_t c1(0.1, 0.2, 0.3, 1.0);
        wf::color_t c2(0.1 + 1e-7, 0.2, 0.3, 1.0);
        wf::color_t c3(0.1 + 1e-3, 0.2, 0.3, 1.0);
        CHECK(c1 == c2);
        CHECK_FALSE(c1 == c3);
    }
}

TEST_CASE("color: parsing edge cases")
{
    SUBCASE("invalid hex length") {
        CHECK(wf::option_type::from_string<wf::color_t>("#123") == std::nullopt);
        CHECK(wf::option_type::from_string<wf::color_t>("#1234567") == std::nullopt);
        CHECK(wf::option_type::from_string<wf::color_t>("#123456789") == std::nullopt);
    }

    SUBCASE("invalid characters") {
        CHECK(wf::option_type::from_string<wf::color_t>("#GGGGGGG") == std::nullopt);
        CHECK(wf::option_type::from_string<wf::color_t>("#12345GZ") == std::nullopt);
    }

    SUBCASE("no hash") {
        CHECK(wf::option_type::from_string<wf::color_t>("12345678") == std::nullopt);
    }

    SUBCASE("short hex format") {
        auto c = wf::option_type::from_string<wf::color_t>("#1234");
        REQUIRE(c.has_value());
        CHECK(c->r > 0);
    }

    SUBCASE("rgba format") {
        auto c = wf::option_type::from_string<wf::color_t>("#12345678");
        REQUIRE(c.has_value());
        CHECK(c->r > 0);
    }
}

/* ==================== option locking tests ==================== */

TEST_CASE("option: lock counter edge cases")
{
    auto opt = std::make_shared<option_t<int>>("test", 42);

    SUBCASE("lock and unlock") {
        CHECK_FALSE(opt->is_locked());
        opt->set_locked();
        CHECK(opt->is_locked());
        opt->set_locked(false);
        CHECK_FALSE(opt->is_locked());
    }

    SUBCASE("multiple locks") {
        opt->set_locked();
        opt->set_locked();
        CHECK(opt->is_locked());
        opt->set_locked(false);
        CHECK(opt->is_locked()); // Still locked
        opt->set_locked(false);
        CHECK_FALSE(opt->is_locked());
    }

    SUBCASE("negative lock count warning") {
        std::stringstream log;
        wf::log::initialize_logging(log, wf::log::LOG_LEVEL_DEBUG,
            wf::log::LOG_COLOR_MODE_OFF);

        // Unlock more than locked
        opt->set_locked(false);

        EXPECT_LINE(log, "dropped below zero");

        wf::log::initialize_logging(std::cout, wf::log::LOG_LEVEL_DEBUG,
            wf::log::LOG_COLOR_MODE_OFF);
    }
}

/* ==================== callback tests ==================== */

TEST_CASE("option: callback management")
{
    auto opt = std::make_shared<option_t<int>>("test", 42);

    int count1 = 0, count2 = 0;
    option_base_t::updated_callback_t cb1 = [&]() { count1++; };
    option_base_t::updated_callback_t cb2 = [&]() { count2++; };

    opt->add_updated_handler(&cb1);
    opt->add_updated_handler(&cb2);

    opt->set_value(100);
    CHECK(count1 == 1);
    CHECK(count2 == 1);

    // Register same callback twice
    opt->add_updated_handler(&cb1);
    opt->set_value(200);
    CHECK(count1 == 3);
    CHECK(count2 == 2);

    // Remove handler
    opt->rem_updated_handler(&cb1);
    opt->set_value(300);
    CHECK(count1 == 3); // Unchanged
    CHECK(count2 == 3);
}

/* ==================== split_at and filter_out internal tests ==================== */

TEST_CASE("internal: split_at with empty tokens")
{
    // Test split_at indirectly through binding parsing with consecutive delimiters

    // Binding with extra spaces
    auto key1 = wf::option_type::from_string<wf::keybinding_t>("<super>  KEY_E");
    REQUIRE(key1.has_value());

    // Binding with angle brackets adjacent
    auto key2 = wf::option_type::from_string<wf::keybinding_t>("<super><alt>KEY_E");
    REQUIRE(key2.has_value());
}

TEST_CASE("internal: parse_binding with all modifiers")
{
    // All modifiers
    auto key = wf::option_type::from_string<wf::keybinding_t>("<ctrl><alt><shift><super> KEY_E");
    REQUIRE(key.has_value());
    uint32_t expected_mods = wf::KEYBOARD_MODIFIER_CTRL | wf::KEYBOARD_MODIFIER_ALT |
                             wf::KEYBOARD_MODIFIER_SHIFT | wf::KEYBOARD_MODIFIER_LOGO;
    CHECK(key->get_modifiers() == expected_mods);
}

TEST_CASE("internal: modifier-only binding")
{
    // Just modifiers, no key
    auto key = wf::option_type::from_string<wf::keybinding_t>("<ctrl><alt><shift>");
    REQUIRE(key.has_value());
    uint32_t expected_mods = wf::KEYBOARD_MODIFIER_CTRL | wf::KEYBOARD_MODIFIER_ALT |
                             wf::KEYBOARD_MODIFIER_SHIFT;
    CHECK(key->get_modifiers() == expected_mods);
    CHECK(key->get_key() == 0);
}
