<?php
/*
** Zabbix
** Copyright (C) 2001-2024 Zabbix SIA
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**/


require_once dirname(__FILE__).'/../../include/func.inc.php';
require_once dirname(__FILE__).'/../include/CTest.php';

class dbConditionStringTest extends CTest {

	public static function provider() {
		return [
			[
				['field', []],
				'1=0'
			],
			[
				['field', ['a']],
				'field=\'a\''
			],
			[
				['field', ['a'], true],
				'field!=\'a\''
			]
		];
	}

	/**
	 * @dataProvider provider
	 */
	public function test($params, $expectedResult) {
		$result = call_user_func_array('dbConditionString', $params);

		$this->assertSame($expectedResult, $result);
	}
}
