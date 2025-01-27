#include "prism.h"
#include <storm/storage/prism/Program.h>
#include <boost/variant.hpp>
#include "src/helpers.h"
#include <storm/storage/expressions/ExpressionManager.h>
#include <storm/storage/jani/Model.h>
#include <storm/storage/jani/Property.h>
#include <storm/generator/NextStateGenerator.h>
#include <storm/generator/Choice.h>
#include <storm/generator/PrismNextStateGenerator.h>
#include "storm/exceptions/NotSupportedException.h"
#include <storm/storage/expressions/SimpleValuation.h>
#include "storm/exceptions/InvalidTypeException.h"
#include "storm/exceptions/InvalidStateException.h"
#include "storm/exceptions/InvalidAccessException.h"


using namespace storm::prism;

template <typename StateType, typename ValueType>
void define_stateGeneration(py::module& m);

void define_prism(py::module& m) {
    py::class_<storm::prism::Program, std::shared_ptr<storm::prism::Program>> program(m, "PrismProgram", "A Prism Program");
    program.def_property_readonly("constants", &Program::getConstants, "Get Program Constants")
    .def_property_readonly("nr_modules", &storm::prism::Program::getNumberOfModules, "Number of modules")
            .def_property_readonly("modules", &storm::prism::Program::getModules, "Modules in the program")
            .def_property_readonly("model_type", &storm::prism::Program::getModelType, "Model type")
            .def_property_readonly("has_undefined_constants", &storm::prism::Program::hasUndefinedConstants, "Flag if program has undefined constants")
            .def_property_readonly("undefined_constants_are_graph_preserving", &storm::prism::Program::undefinedConstantsAreGraphPreserving, "Flag if the undefined constants do not change the graph structure")
            .def("substitute_constants", &Program::substituteConstants, "Substitute constants within program")
            .def("define_constants", &Program::defineUndefinedConstants, "Define constants")
            .def("restrict_commands", &Program::restrictCommands, "Restrict commands")
            .def("simplify", &Program::simplify, "Simplify")
            .def("used_constants",&Program::usedConstants, "Compute Used Constants")
            .def_property_readonly("hasUndefinedConstants", &Program::hasUndefinedConstants, "Does the program have undefined constants?")
            .def_property_readonly("isDeterministicModel", &Program::isDeterministicModel, "Does the program describe a deterministic model?")
            .def_property_readonly("expression_manager", &Program::getManager, "Get the expression manager for expressions in this program")
            .def("to_jani", [](storm::prism::Program const& program, std::vector<storm::jani::Property> const& properties, bool allVariablesGlobal, std::string suffix) {
                    return program.toJani(properties, allVariablesGlobal, suffix);
                }, "Transform to Jani program", py::arg("properties"), py::arg("all_variables_global") = true, py::arg("suffix") = "")
            .def("__str__", &streamToString<storm::prism::Program>)
            .def_property_readonly("variables", &Program::getAllExpressionVariables, "Get all Expression Variables used by the program")
            .def("get_label_expression", [](storm::prism::Program const& program, std::string const& label){
                return program.getLabelExpression(label);
            }, "Get the expression of the given label.", py::arg("label"))
            ;

    py::class_<Module> module(m, "PrismModule", "A module in a Prism program");
    module.def_property_readonly("commands", [](Module const& module) {return module.getCommands();}, "Commands in the module")
            .def_property_readonly("name", &Module::getName, "Name of the module")
            .def_property_readonly("integer_variables", &Module::getIntegerVariables, "All integer Variables of this module")
            .def_property_readonly("boolean_variables", &Module::getBooleanVariables, "All boolean Variables of this module")
            .def("__str__", &streamToString<Module>)
            ;

    py::class_<Command> command(m, "PrismCommand", "A command in a Prism program");
    command.def_property_readonly("global_index", &Command::getGlobalIndex, "Get global index")
            .def_property_readonly("guard_expression", &Command::getGuardExpression, "Get guard expression")
            .def_property_readonly("updates", [](Command const& command) {
                    return command.getUpdates();
                }, "Updates in the command")
            .def("__str__", &streamToString<Command>)
            ;

    py::class_<Update> update(m, "PrismUpdate", "An update in a Prism command");
    update.def_property_readonly("assignments", [](Update const& update) {
                    return update.getAssignments();
                }, "Assignments in the update")
           .def_property_readonly("probability_expression", &Update::getLikelihoodExpression, "The probability expression for this update")
           .def("__str__", &streamToString<Update>)
           ;

    py::class_<Assignment> assignment(m, "PrismAssignment", "An assignment in prism");
    assignment.def_property_readonly("variable", &Assignment::getVariable, "Variable that is updated")
            .def_property_readonly("expression", &Assignment::getExpression, "Expression for the update")
            .def("__str__", &streamToString<Assignment>)
            ;



    // PrismType
    py::enum_<storm::prism::Program::ModelType>(m, "PrismModelType", "Type of the prism model")
            .value("DTMC", storm::prism::Program::ModelType::DTMC)
            .value("CTMC", storm::prism::Program::ModelType::CTMC)
            .value("MDP", storm::prism::Program::ModelType::MDP)
            .value("CTMDP", storm::prism::Program::ModelType::CTMDP)
            .value("MA", storm::prism::Program::ModelType::MA)
            .value("UNDEFINED", storm::prism::Program::ModelType::UNDEFINED)
            ;


    py::class_<Constant, std::shared_ptr<Constant>> constant(m, "Constant", "A constant in a Prism program");
    constant.def_property_readonly("name", &Constant::getName, "Constant name")
            .def_property_readonly("defined", &Constant::isDefined, "Is the constant defined?")
            .def_property_readonly("type", &Constant::getType, "The type of the constant")
            .def_property_readonly("expression_variable", &Constant::getExpressionVariable, "Expression variable")
            ;



    py::class_<Variable, std::shared_ptr<Variable>> variable(m, "Prism_Variable", "A program variable in a Prism program");
    variable.def_property_readonly("name", &Variable::getName, "Variable Name")
            .def_property_readonly("initial_value_expression", &Variable::getInitialValueExpression, "The expression represented the initial value of the variable")
            .def_property_readonly("expression_variable", &Variable::getExpressionVariable, "The expression variable corresponding to the variable")
            ;

    py::class_<IntegerVariable, std::shared_ptr<IntegerVariable>> integer_variable(m, "Prism_Integer_Variable", variable, "A program integer variable in a Prism program");
    integer_variable.def_property_readonly("lower_bound_expression", &IntegerVariable::getLowerBoundExpression, "The the lower bound expression of this integer variable")
                    .def_property_readonly("upper_bound_expression", &IntegerVariable::getUpperBoundExpression, "The the upper bound expression of this integer variable")
                    .def("__str__", &streamToString<IntegerVariable>)
                    ;

    py::class_<BooleanVariable, std::shared_ptr<BooleanVariable>> boolean_variable(m, "Prism_Boolean_Variable", variable, "A program boolean variable in a Prism program");
    boolean_variable.def("__str__", &streamToString<BooleanVariable>);

    define_stateGeneration<uint32_t, storm::RationalNumber>(m);
}

class ValuationMapping {
    public:
    std::map<storm::expressions::Variable, bool> booleanValues;
    std::map<storm::expressions::Variable, int_fast64_t> integerValues;
    std::map<storm::expressions::Variable, double> rationalValues;

    ValuationMapping(storm::prism::Program const& program,
                     storm::expressions::SimpleValuation valuation) {
        auto const& variables = program.getManager().getVariables();
        for (auto const& variable : variables) {
            if (variable.hasBooleanType()) {
                booleanValues[variable] = valuation.getBooleanValue(variable);
            } else if (variable.hasIntegerType()) {
                integerValues[variable] = valuation.getIntegerValue(variable);
            } else if (variable.hasRationalType()) {
                rationalValues[variable] = valuation.getRationalValue(variable);
            } else {
                STORM_LOG_THROW(false, storm::exceptions::InvalidTypeException,
                        "Unexpected variable type.");
            }
        }
    }

    std::string toString() const {
        std::vector<std::string> strs;
        for (auto const& value : booleanValues) {
            std::stringstream sstr;
            sstr << value.first.getName() + "=";
            sstr << value.second;
            strs.push_back(sstr.str());
        }
        for (auto const& value : integerValues) {
            std::stringstream sstr;
            sstr << value.first.getName() + "=";
            sstr << value.second;
            strs.push_back(sstr.str());
        }
        for (auto const& value : rationalValues) {
            std::stringstream sstr;
            sstr << value.first.getName() + "=";
            sstr << value.second;
            strs.push_back(sstr.str());
        }
        return "[" + boost::join(strs, ",") + "]";
    }
};

template <typename StateType, typename ValueType>
class StateGenerator {
    typedef std::unordered_map<StateType, storm::generator::CompressedState> IdToStateMap;
    typedef std::vector<std::pair<StateType, ValueType>> distribution_type;
    typedef distribution_type choice_type; // currently we just collapse choices into distributions
    typedef std::vector<choice_type> choice_list_type;

    storm::prism::Program const& program;
    storm::generator::PrismNextStateGenerator<ValueType, StateType> generator;
    std::function<StateType (storm::generator::CompressedState const&)> stateToIdCallback;
    // this needs to be below the generator attribute,
    // because its initialization depends on the generator being initialized.
    // #justcppthings
    storm::storage::sparse::StateStorage<StateType> stateStorage;
    bool hasComputedInitialStates = false;
    IdToStateMap stateMap;
    boost::optional<StateType> currentStateIndex;

    public:
        StateGenerator(storm::prism::Program const& program_) : program(program_), generator(program_), stateStorage(generator.getStateSize()) {
            stateToIdCallback = [this] (storm::generator::CompressedState const& state) -> StateType {
                StateType newIndex = stateStorage.getNumberOfStates();
                std::pair<StateType, std::size_t> indexBucketPair = stateStorage.stateToId.findOrAddAndGetBucket(state, newIndex);
                StateType index = indexBucketPair.first;
                stateMap[index] = state;
                return index;
            };
        }

        StateType loadInitialState() {
            if (!hasComputedInitialStates) {
                stateStorage.initialStateIndices = generator.getInitialStates(stateToIdCallback);
                hasComputedInitialStates = true;
            }
            STORM_LOG_THROW(stateStorage.initialStateIndices.size() == 1, storm::exceptions::NotSupportedException, "Currently only models with one initial state are supported.");
            StateType initialStateIndex = stateStorage.initialStateIndices.front();
            load(initialStateIndex);
            return initialStateIndex;
        }

        void load(StateType stateIndex) {
            if (currentStateIndex && *currentStateIndex == stateIndex) {
                return;
            }
            auto search = stateMap.find(stateIndex);
            if (search == stateMap.end()) {
                STORM_LOG_THROW(false, storm::exceptions::InvalidAccessException,
                        "state id not found");
            }
            generator.load(search->second);
            currentStateIndex = stateIndex;
        }

        ValuationMapping currentStateToValuation() {
            if (!currentStateIndex) {
                STORM_LOG_THROW(false, storm::exceptions::InvalidStateException,
                        "Initial state not initialized");
            }
            auto valuation = generator.toValuation(stateMap[*currentStateIndex]);
            return ValuationMapping(program, valuation);
        }

        bool satisfies(storm::expressions::Expression const& expression) {
            return generator.satisfies(expression);
        }

        choice_list_type expand() {
            if (!hasComputedInitialStates) {
                STORM_LOG_THROW(false, storm::exceptions::InvalidStateException,
                        "Initial state not initialized");
            }
            choice_list_type choices_result;
            auto behavior = generator.expand(stateToIdCallback);
            for (auto choice : behavior.getChoices()) {
                choices_result.push_back(choice_type(choice.begin(), choice.end()));
            }

            return choices_result;
        }

};

template <typename StateType, typename ValueType>
void define_stateGeneration(py::module& m) {
    py::class_<ValuationMapping, std::shared_ptr<ValuationMapping>> valuation_mapping(m, "ValuationMapping", "A valuation mapping for a state consists of a mapping from variable to value for each of the three types.");
    valuation_mapping
        .def_readonly("boolean_values", &ValuationMapping::booleanValues)
        .def_readonly("integer_values", &ValuationMapping::integerValues)
        .def_readonly("rational_values", &ValuationMapping::rationalValues)
        .def("__str__", &ValuationMapping::toString);

    py::class_<StateGenerator<StateType, ValueType>, std::shared_ptr<StateGenerator<StateType, ValueType>>> state_generator(m, "StateGenerator", R"doc(
        Interactively explore states using Storm's PrismNextStateGenerator.

        :ivar program: A PRISM program.
    )doc");
    state_generator
        .def(py::init<storm::prism::Program const&>())
        .def("load_initial_state", &StateGenerator<StateType, ValueType>::loadInitialState, R"doc(
            Loads the (unique) initial state.
            Multiple initial states are not supported.

            :rtype: the ID of the initial state.
        )doc")
        .def("load", &StateGenerator<StateType, ValueType>::load, R"doc(
            :param state_id: The ID of the state to load.
        )doc")
        .def("current_state_to_valuation", &StateGenerator<StateType, ValueType>::currentStateToValuation, R"doc(
            Return a valuation for the currently loaded state.

            :rtype: stormpy.ValuationMapping
        )doc")
        .def("current_state_satisfies", &StateGenerator<StateType, ValueType>::satisfies, R"doc(
            Check if the currently loaded state satisfies the given expression.

            :param stormpy.Expression expression: The expression to check against.
            :rtype: bool
        )doc")
        .def("expand", &StateGenerator<StateType, ValueType>::expand, R"doc(
            Expand the currently loaded state and return its successors.

            :rtype: [[(state_id, probability)]]
        )doc");
}
