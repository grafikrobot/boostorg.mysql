//
// Copyright (c) 2019-2020 Ruben Perez Hidalgo (rubenperez038 at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "serialization_test_common.hpp"
#include "boost/mysql/detail/protocol/common_messages.hpp"

using namespace boost::mysql::test;
using namespace boost::mysql::detail;
using boost::mysql::collation;

namespace
{

INSTANTIATE_TEST_SUITE_P(PacketHeader, FullSerializationTest, ::testing::Values(
	serialization_testcase(packet_header{int3(3), int1(0)}, {0x03, 0x00, 0x00, 0x00}, "small_packet_seqnum_0"),
	serialization_testcase(packet_header{int3(9), int1(2)}, {0x09, 0x00, 0x00, 0x02}, "small_packet_seqnum_not_0"),
	serialization_testcase(packet_header{int3(0xcacbcc), int1(0xfa)}, {0xcc, 0xcb, 0xca, 0xfa}, "big_packet_seqnum_0"),
	serialization_testcase(packet_header{int3(0xffffff), int1(0xff)}, {0xff, 0xff, 0xff, 0xff}, "max_packet_max_seqnum")
), test_name_generator);

INSTANTIATE_TEST_SUITE_P(OkPacket, DeserializeTest, ::testing::Values(
	serialization_testcase(ok_packet{
		int_lenenc(4), // affected rows
		int_lenenc(0), // last insert ID
		int2(static_cast<std::uint16_t>(SERVER_STATUS_AUTOCOMMIT | SERVER_QUERY_NO_INDEX_USED)), // server status
		int2(0), // warnings
		string_lenenc("Rows matched: 5  Changed: 4  Warnings: 0")
	}, {
		0x04, 0x00, 0x22, 0x00, 0x00, 0x00, 0x28, 0x52, 0x6f, 0x77, 0x73,
		0x20, 0x6d, 0x61, 0x74, 0x63, 0x68, 0x65, 0x64, 0x3a, 0x20, 0x35, 0x20, 0x20, 0x43, 0x68, 0x61,
		0x6e, 0x67, 0x65, 0x64, 0x3a, 0x20, 0x34, 0x20, 0x20, 0x57, 0x61, 0x72, 0x6e, 0x69, 0x6e, 0x67,
		0x73, 0x3a, 0x20, 0x30
	}, "successful_update"),

	serialization_testcase(ok_packet{
		int_lenenc(1), // affected rows
		int_lenenc(6), // last insert ID
		int2(static_cast<std::uint16_t>(SERVER_STATUS_AUTOCOMMIT)), // server status
		int2(0), // warnings
		string_lenenc("")  // no message
	},{
		0x01, 0x06, 0x02, 0x00, 0x00, 0x00
	}, "successful_insert"),

	serialization_testcase(ok_packet{
		int_lenenc(0), // affected rows
		int_lenenc(0), // last insert ID
		int2(static_cast<std::uint16_t>(SERVER_STATUS_AUTOCOMMIT)), // server status
		int2(0), // warnings
		string_lenenc("")  // no message
	}, {
		0x00, 0x00, 0x02, 0x00, 0x00, 0x00
	}, "successful_login")
), test_name_generator);

INSTANTIATE_TEST_SUITE_P(ErrPacket, DeserializeTest, ::testing::Values(
	serialization_testcase(err_packet{
		int2(1049), // eror code
		string_fixed<1>{0x23}, // sql state marker
		string_fixed<5>{'4', '2', '0', '0', '0'}, // sql state
		string_eof("Unknown database 'a'") // err msg
	}, {
		0x19, 0x04, 0x23, 0x34, 0x32, 0x30, 0x30, 0x30, 0x55, 0x6e, 0x6b,
		0x6e, 0x6f, 0x77, 0x6e, 0x20, 0x64, 0x61, 0x74,
		0x61, 0x62, 0x61, 0x73, 0x65, 0x20, 0x27, 0x61, 0x27
	}, "wrong_use_database"),

	serialization_testcase(err_packet{
		int2(1146), // eror code
		string_fixed<1>{0x23}, // sql state marker
		string_fixed<5>{'4', '2', 'S', '0', '2'}, // sql state
		string_eof("Table 'awesome.unknown' doesn't exist") // err msg
	}, {
		0x7a, 0x04, 0x23, 0x34, 0x32, 0x53, 0x30, 0x32,
		0x54, 0x61, 0x62, 0x6c, 0x65, 0x20, 0x27, 0x61,
		0x77, 0x65, 0x73, 0x6f, 0x6d, 0x65, 0x2e, 0x75,
		0x6e, 0x6b, 0x6e, 0x6f, 0x77, 0x6e, 0x27, 0x20,
		0x64, 0x6f, 0x65, 0x73, 0x6e, 0x27, 0x74, 0x20,
		0x65, 0x78, 0x69, 0x73, 0x74
	}, "unknown_table"),

	serialization_testcase(err_packet{
		int2(1045), // error code
		string_fixed<1>{0x23}, // SQL state marker
		string_fixed<5>{'2', '8', '0', '0', '0'}, // SQL state
		string_eof("Access denied for user 'root'@'localhost' (using password: YES)")
	}, {
	  0x15, 0x04, 0x23, 0x32, 0x38, 0x30, 0x30, 0x30,
	  0x41, 0x63, 0x63, 0x65, 0x73, 0x73, 0x20, 0x64,
	  0x65, 0x6e, 0x69, 0x65, 0x64, 0x20, 0x66, 0x6f,
	  0x72, 0x20, 0x75, 0x73, 0x65, 0x72, 0x20, 0x27,
	  0x72, 0x6f, 0x6f, 0x74, 0x27, 0x40, 0x27, 0x6c,
	  0x6f, 0x63, 0x61, 0x6c, 0x68, 0x6f, 0x73, 0x74,
	  0x27, 0x20, 0x28, 0x75, 0x73, 0x69, 0x6e, 0x67,
	  0x20, 0x70, 0x61, 0x73, 0x73, 0x77, 0x6f, 0x72,
	  0x64, 0x3a, 0x20, 0x59, 0x45, 0x53, 0x29
	}, "failed_login")
), test_name_generator);

// Column definition
INSTANTIATE_TEST_SUITE_P(ColumnDefinition, DeserializeSpaceTest, testing::Values(
	serialization_testcase(column_definition_packet{
		string_lenenc("def"), //catalog
		string_lenenc("awesome"), // schema (database)
		string_lenenc("test_table"), // table
		string_lenenc("test_table"), // physical table
		string_lenenc("id"), // field name
		string_lenenc("id"), // physical field name
		collation::binary,
		int4(11), // length
		protocol_field_type::long_,
		int2(
			column_flags::not_null |
			column_flags::pri_key |
			column_flags::auto_increment |
			column_flags::part_key
		),
		int1(0) // decimals
	}, {
		0x03, 0x64, 0x65, 0x66, 0x07, 0x61, 0x77, 0x65,
		0x73, 0x6f, 0x6d, 0x65, 0x0a, 0x74, 0x65, 0x73,
		0x74, 0x5f, 0x74, 0x61, 0x62, 0x6c, 0x65, 0x0a,
		0x74, 0x65, 0x73, 0x74, 0x5f, 0x74, 0x61, 0x62,
		0x6c, 0x65, 0x02, 0x69, 0x64, 0x02, 0x69, 0x64,
		0x0c, 0x3f, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x03,
		0x03, 0x42, 0x00, 0x00, 0x00
	}, "numeric_auto_increment_primary_key"),
	serialization_testcase(column_definition_packet{
		string_lenenc("def"), //catalog
		string_lenenc("awesome"), // schema (database)
		string_lenenc("child"), // table
		string_lenenc("child_table"), // physical table
		string_lenenc("field_alias"), // field name
		string_lenenc("field_varchar"), // physical field name
		collation::utf8_general_ci,
		int4(765), // length
		protocol_field_type::var_string,
		int2(), // no column flags
		int1(0) // decimals
	}, {
		0x03, 0x64, 0x65, 0x66, 0x07, 0x61, 0x77, 0x65,
		0x73, 0x6f, 0x6d, 0x65, 0x05, 0x63, 0x68, 0x69,
		0x6c, 0x64, 0x0b, 0x63, 0x68, 0x69, 0x6c, 0x64,
		0x5f, 0x74, 0x61, 0x62, 0x6c, 0x65, 0x0b, 0x66,
		0x69, 0x65, 0x6c, 0x64, 0x5f, 0x61, 0x6c, 0x69,
		0x61, 0x73, 0x0d, 0x66, 0x69, 0x65, 0x6c, 0x64,
		0x5f, 0x76, 0x61, 0x72, 0x63, 0x68, 0x61, 0x72,
		0x0c, 0x21, 0x00, 0xfd, 0x02, 0x00, 0x00, 0xfd,
		0x00, 0x00, 0x00, 0x00, 0x00
	}, "varchar_field_aliased_field_and_table_names_join"),

	serialization_testcase(column_definition_packet{
		string_lenenc("def"), //catalog
		string_lenenc("awesome"), // schema (database)
		string_lenenc("test_table"), // table
		string_lenenc("test_table"), // physical table
		string_lenenc("field_float"), // field name
		string_lenenc("field_float"), // physical field name
		collation::binary, // binary
		int4(12), // length
		protocol_field_type::float_,
		int2(), // no column flags
		int1(31) // decimals
	}, {
		0x03, 0x64, 0x65, 0x66, 0x07, 0x61, 0x77, 0x65,
		0x73, 0x6f, 0x6d, 0x65, 0x0a, 0x74, 0x65, 0x73,
		0x74, 0x5f, 0x74, 0x61, 0x62, 0x6c, 0x65, 0x0a,
		0x74, 0x65, 0x73, 0x74, 0x5f, 0x74, 0x61, 0x62,
		0x6c, 0x65, 0x0b, 0x66, 0x69, 0x65, 0x6c, 0x64,
		0x5f, 0x66, 0x6c, 0x6f, 0x61, 0x74, 0x0b, 0x66,
		0x69, 0x65, 0x6c, 0x64, 0x5f, 0x66, 0x6c, 0x6f,
		0x61, 0x74, 0x0c, 0x3f, 0x00, 0x0c, 0x00, 0x00,
		0x00, 0x04, 0x00, 0x00, 0x1f, 0x00, 0x00
	}, "float_field")
), test_name_generator);


} // anon namespace
