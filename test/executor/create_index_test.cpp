//===----------------------------------------------------------------------===//
//
//                         Peloton
//
// delete_test.cpp
//
// Identification: test/executor/create_index_test.cpp
//
// Copyright (c) 2015-16, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <include/tcop/tcop.h>
#include <cstdio>

#include "catalog/catalog.h"
#include "common/harness.h"
#include "common/logger.h"
#include "common/statement.h"
#include "executor/create_executor.h"
#include "executor/delete_executor.h"
#include "executor/insert_executor.h"
#include "executor/plan_executor.h"
#include "executor/update_executor.h"
#include "optimizer/optimizer.h"
#include "parser/postgresparser.h"
#include "planner/create_plan.h"
#include "planner/delete_plan.h"
#include "planner/insert_plan.h"
#include "planner/plan_util.h"
#include "planner/update_plan.h"
#include "tcop/tcop.h"

#include "gtest/gtest.h"

namespace peloton {
namespace test {

//===--------------------------------------------------------------------===//
// Catalog Tests
//===--------------------------------------------------------------------===//

class CreateIndexTests : public PelotonTest {};

TEST_F(CreateIndexTests, CreatingIndex) {
  LOG_INFO("Bootstrapping...");
  auto& txn_manager = concurrency::TransactionManagerFactory::GetInstance();
  auto txn = txn_manager.BeginTransaction();
  catalog::Catalog::GetInstance()->CreateDatabase(DEFAULT_DB_NAME, txn);
  txn_manager.CommitTransaction(txn);
  LOG_INFO("Bootstrapping completed!");

  optimizer::Optimizer optimizer;
  auto& traffic_cop = tcop::TrafficCop::GetInstance();

  // Create a table first
  txn = txn_manager.BeginTransaction();
  LOG_INFO("Creating table");
  LOG_INFO(
      "Query: CREATE TABLE department_table(dept_id INT PRIMARY KEY,student_id "
      "INT, dept_name TEXT);");
  std::unique_ptr<Statement> statement;
  statement.reset(new Statement("CREATE",
                                "CREATE TABLE department_table(dept_id INT "
                                "PRIMARY KEY, student_id INT, dept_name "
                                "TEXT);"));

  auto& peloton_parser = parser::PostgresParser::GetInstance();

  LOG_INFO("Building parse tree...");
  auto create_stmt = peloton_parser.BuildParseTree(
      "CREATE TABLE department_table(dept_id INT PRIMARY KEY, student_id INT, "
      "dept_name TEXT);");
  LOG_INFO("Building parse tree completed!");

  LOG_INFO("Building plan tree...");
  statement->SetPlanTree(optimizer.BuildPelotonPlanTree(create_stmt));
  LOG_INFO("Building plan tree completed!");

  std::vector<type::Value> params;
  std::vector<StatementResult> result;
  LOG_INFO("Executing plan...\n%s",
           planner::PlanUtil::GetInfo(statement->GetPlanTree().get()).c_str());
  std::vector<int> result_format;
  result_format =
      std::vector<int>(statement->GetTupleDescriptor().size(), 0);
  executor::ExecuteResult status = traffic_cop.ExecuteStatementPlan(
      statement->GetPlanTree(), params, result, result_format);
  LOG_INFO("Statement executed. Result: %s",
           ResultTypeToString(status.m_result).c_str());
  LOG_INFO("Table Created");
  txn_manager.CommitTransaction(txn);

  EXPECT_EQ(catalog::Catalog::GetInstance()
                ->GetDatabaseWithName(DEFAULT_DB_NAME)
                ->GetTableCount(),
            1);

  // Inserting a tuple end-to-end
  txn = txn_manager.BeginTransaction();
  LOG_INFO("Inserting a tuple...");
  LOG_INFO(
      "Query: INSERT INTO department_table(dept_id,student_id ,dept_name) "
      "VALUES (1,52,'hello_1');");
  statement.reset(new Statement("INSERT",
                                "INSERT INTO department_table(dept_id, "
                                "student_id, dept_name) VALUES "
                                "(1,52,'hello_1');"));

  LOG_INFO("Building parse tree...");
  auto insert_stmt = peloton_parser.BuildParseTree(
      "INSERT INTO department_table(dept_id,student_id,dept_name) VALUES "
      "(1,52,'hello_1');");
  LOG_INFO("Building parse tree completed!");

  LOG_INFO("Building plan tree...");
  statement->SetPlanTree(optimizer.BuildPelotonPlanTree(insert_stmt));
  LOG_INFO("Building plan tree completed!\n%s",
           planner::PlanUtil::GetInfo(statement->GetPlanTree().get()).c_str());

  LOG_INFO("Executing plan...");
  result_format =
      std::vector<int>(statement->GetTupleDescriptor().size(), 0);
  status = traffic_cop.ExecuteStatementPlan(statement->GetPlanTree(),
                                            params, result, result_format);
  LOG_INFO("Statement executed. Result: %s",
           ResultTypeToString(status.m_result).c_str());
  LOG_INFO("Tuple inserted!");
  txn_manager.CommitTransaction(txn);

  // Now Updating end-to-end
  txn = txn_manager.BeginTransaction();
  LOG_INFO("Creating and Index");
  LOG_INFO("Query: CREATE INDEX saif ON department_table (student_id);");
  statement.reset(new Statement(
      "CREATE", "CREATE INDEX saif ON department_table (student_id);"));

  LOG_INFO("Building parse tree...");
  auto update_stmt = peloton_parser.BuildParseTree(
      "CREATE INDEX saif ON department_table (student_id);");
  LOG_INFO("Building parse tree completed!");

  LOG_INFO("Building plan tree...");
  statement->SetPlanTree(optimizer.BuildPelotonPlanTree(update_stmt));
  LOG_INFO("Building plan tree completed!\n%s",
           planner::PlanUtil::GetInfo(statement->GetPlanTree().get()).c_str());

  LOG_INFO("Executing plan...");
  result_format =
      std::vector<int>(statement->GetTupleDescriptor().size(), 0);
  status = traffic_cop.ExecuteStatementPlan(statement->GetPlanTree(),
                                            params, result, result_format);
  LOG_INFO("Statement executed. Result: %s",
           ResultTypeToString(status.m_result).c_str());
  LOG_INFO("INDEX CREATED!");
  txn_manager.CommitTransaction(txn);

  auto target_table_ = catalog::Catalog::GetInstance()->GetTableWithName(
      DEFAULT_DB_NAME, "department_table");
  // Expected 2 , Primary key index + created index
  EXPECT_EQ(target_table_->GetIndexCount(), 2);

  // free the database just created
  txn = txn_manager.BeginTransaction();
  catalog::Catalog::GetInstance()->DropDatabaseWithName(DEFAULT_DB_NAME, txn);
  txn_manager.CommitTransaction(txn);
}

}  // namespace test
}  // namespace peloton
