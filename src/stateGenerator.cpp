#include <stateGenerator.hpp>
#include <queue>
#include <set>
#include <iostream>
#include <climits>
#include <store.hpp>

using namespace std;

/******** State ************/

class FollowSet {
	vector<FollowSet *> parents;
	std::set<int> own;

	public:

	void insert(int termId) {
		own.insert(termId);
	}

	void addParent(FollowSet *p) {
		parents.push_back(p);
	}

	set<int> ids() const {
		std::set<int> s = own;
		for(auto parent : parents) {
			for(int id : parent->ids()) {
				s.insert(id);
			}
		}
		return s;
	}
};

static string& EMPTY = *(new string(""));
#define EPSILON_STR "ε"

class StateComponent {
	friend class State;

	FollowSet * const _follow;
	const Rule &_rule;
	int _idx;

	public:
	StateComponent(const Rule &r) : _rule(r), _idx(0), _follow(&Store<FollowSet>::newInstance()) {}
	StateComponent(const Rule& r, FollowSet *followSet) : _rule(r), _idx(0), _follow(&Store<FollowSet>::newInstance(*followSet)) {}
	
	string toString() const {
		return "[" + to_string(_rule.id()) + "," + to_string(_idx) + "]";
	}
	
	const Rule &rule() const {
		return _rule;
	}
	
	FollowSet &followSet() const {
		return *_follow;
	}

	int idx() const {
		return _idx;
	}
};

struct Cmp {
	bool operator()(const StateComponent& l, const StateComponent& r) const {
		return l.toString() < r.toString();
	}
};

class State {
	map<string, set<StateComponent, Cmp>> expandable;
	set<StateComponent, Cmp> reducible;
	queue<StateComponent> unprocessed;

	string id;

	State(){}

	void addKernelComponent(const StateComponent& sc){
		const Term* cur = sc.rule().getTerm(sc.idx());
		if(cur == nullptr){
			reducible.insert(sc);
		}else{
			expandable[cur->getName()].insert(sc);
			unprocessed.push(sc);
		}
		id += sc.toString();
	}

	string scToString(const StateComponent &sc) const {
		string r;
		r += sc.rule().lhs()->getName() + " ->";
		int i = 0;
		while(true){
			auto t = sc.rule().getTerm(i);
			if(t == nullptr)	break;

			if(i == sc.idx()){
				r += ".";
			}else{
				r += " ";
			}

			r += t->getName();
			i++;
		}

		if(sc.rule().getTerms().size() == sc.idx()){
			r += ".";
			if(sc.idx() == 0) {
				r += EPSILON_STR;
			}
		}

		r += " [";
		for(auto str: sc.followSet().ids()){
			r += to_string(str) + ",";
		}
		r += "]\n";
		return r;
	}

	public:
	static State createStartState(const StateComponent& sc){
		State s;
		s.addKernelComponent(sc);
		return s;
	}

	const string& strId() const {
		return id;
	}

	bool addUniqueComponent(const StateComponent& sc){
		const Term* cur = sc.rule().getTerm(sc.idx());
		if(cur == nullptr && reducible.find(sc) != reducible.end()){
			FollowSet &f = reducible.find(sc)->followSet();
			for(auto& str: sc.followSet().ids()){
				f.insert(str);
			}
			return false;
		}
		if(cur == nullptr){
			reducible.insert(sc);
			unprocessed.push(sc);
			return true;
		}
		auto itr1 = expandable.find(cur->getName());
		if(itr1  != expandable.end() && itr1->second.find(sc) != itr1->second.end()){
			FollowSet &f = itr1->second.find(sc)->followSet();
			for(auto& str: sc.followSet().ids()){
				f.insert(str);
			}
			return false;
		}
		expandable[cur->getName()].insert(sc);
		unprocessed.push(sc);
		return true;
	}

	const vector<pair<string, State>> shifts() const {
		vector<pair<string, State>> res;
		for(auto def: expandable){
			State s;
			res.emplace_back(def.first, s);
			for(auto sc: def.second){
				sc._idx++;
				res[res.size()-1].second.addKernelComponent(sc);
			}
		}
		return res;
	}

	const set<StateComponent, Cmp> &reduces() const {
		return reducible;
	}

	queue<StateComponent> &processingQueue(){
		return unprocessed;
	}

	string toString() const {
		string r;
		for(auto p : expandable) {
			for(auto sc : p.second) {
				r += scToString(sc);
			}
		}
	
		for(auto sc: reducible) {
			r += scToString(sc);
		}

		return r;
	}

	void update(State &s) {
		if(s.strId() == strId()){
			for(auto p: s.expandable) {
				for(auto sc: p.second) {
					addUniqueComponent(sc);
				}
			}
			for(auto sc: s.reducible) {
				addUniqueComponent(sc);
			}
		}
	}

};


/****** StateStore ********/

int StateGenerator::StateStore::addState(State& state){
	const string& str = state.strId();
	if(!contains(state)){
		states.push_back(&Store<State>::newInstance(state));
		return mp[str] = mp.size();
	}
	return mp[str];
}

bool StateGenerator::StateStore::contains(State& state){
	const string& str = state.strId();
	return mp.find(str) != mp.end();
}

int StateGenerator::StateStore::getId(State& state){
	if(!contains(state))	return -1;
	return mp[state.strId()];
}

State& StateGenerator::StateStore::referenceTo(int id){
	return *(states[id]);
}

int StateGenerator::StateStore::size(){
	return states.size();
}

void StateGenerator::StateStore::reset(){
	mp.clear();
	states.clear();
	Store<State>::reset();
}


/******** StateGenerator *********/

State& StateGenerator::computeClosure(State& kernel){
	auto& q = kernel.processingQueue();
	while(!q.empty()){
		auto& sc = q.front();
		if(sc.rule().getTerm(sc.idx()) == nullptr){
			q.pop();
			continue;
		}
		if(sc.rule().getTerm(sc.idx())->type() == Term::TERMINAL){
			q.pop();
			continue;
		}

		FollowSet *followSet = &Store<FollowSet>::newInstance();
		int offset = 1;
		while(true){
			if(sc.rule().getTerm(sc.idx()+offset) == nullptr){
				followSet->addParent(&sc.followSet());
				break;
			} else {
				bool epsilon = false;
				for(auto& str: firstOf(_ast.termStore.query(sc.rule().getTerm(sc.idx()+offset)))){
					if(str == EPSILON_STR){
						epsilon = true;
						continue;
					}
					followSet->insert(_ast.termStore.query(str));
				}
				if(!epsilon)	break;
			}
			offset++;
		}

		for(auto rule: _ast.getDefinition(sc.rule().getTerm(sc.idx())->getName())){
			StateComponent scc(*rule, followSet);
			kernel.addUniqueComponent(scc);
		}

		q.pop();
	}

	return kernel;
}

bool StateGenerator::generateStateTable(){
	//	Bootstrap
	StateComponent sc(*_ast.getDefinition(_ast.startSymbol())[0]);
	sc.followSet().insert(_ast.termStore.query("$"));
	State s = State::createStartState(sc);

	queue<int> q;
	q.push(store.addState(s));

	int maxID = q.front();

	// Loop
	while(!q.empty()){
		_table.resize(maxID+1);

		auto from = q.front(); q.pop();
		State& state = store.referenceTo(from);
				
		computeClosure(state);

		_table[from].resize(_ast.termStore.size());
		for(int i = 0; i < _ast.termStore.size(); i++){
			_table[from][i].type = Action::REJECT;
			_table[from][i].val = -1;
		}
		
		for(auto pairs: state.shifts()){
			auto& step = pairs.first;
			auto& kernel = pairs.second;

			if(!store.contains(kernel)){
				q.push(store.addState(kernel));
			} else {
				computeClosure(kernel);
				State &s = store.referenceTo(store.getId(kernel));
				s.update(kernel);
			}
			int to = store.getId(kernel);
			maxID = max(to, maxID);

			int termID = _ast.termStore.query(step);
			auto& term = _ast.termStore.query(termID);
			if(term.type() == Term::NONTERMINAL){
				_table[from][termID].type = Action::GOTO;
			} else {
				_table[from][termID].type = Action::SHIFT;
			}
			_table[from][termID].val = to;
		}
	}

	for(int i = 0; i < store.size(); i++) {
		State &s = store.referenceTo(i);
		int from = i;

		cout << "State " << from << ":\n";
		cout << s.toString() << endl;

		for(auto& sc : s.reduces()){
			int ruleId = sc.rule().id();
			for(auto& on: sc.followSet().ids()){
				if(_table[from][on].type != Action::REJECT){
					return false;
				}
				_table[from][on].type = Action::REDUCE;
				_table[from][on].val = ruleId;
			}
		}
	}

	store.reset();
	cout << "Step -1\n";
	Store<StateComponent>::reset();
	cout << "Step -2\n";
	Store<FollowSet>::reset();
	cout << "Step -3\n";

	return true;

}

#define UNDER_PROCESS 1
#define NOT_PROCESSED 2
#define PROCESSED 3

const set<string> &StateGenerator::firstOf(int id){
	static vector<set<string>> firsts;
	static vector<int> calculated;
	if(firsts.size() == 0){
		firsts.resize(_ast.termStore.size(), set<string>());
		calculated.resize(_ast.termStore.size(), NOT_PROCESSED);
	}

	if(calculated[id] == PROCESSED){
		return firsts[id];
	}
	calculated[id] = UNDER_PROCESS;
	firsts[id].clear();

	const Term &term = _ast.termStore.query(id);
	if(term.type() == Term::TERMINAL){
		firsts[id].insert(term.getName());
		calculated[id] = PROCESSED;
		return firsts[id];
	}

	bool processed = true;
	auto rules = _ast.getDefinition(term.getName());
	bool epsilon1 = false, epsilon2 = false, epsilon = false, epsilonAssumption = false;
	set<string> ifEpsilon;
	for(auto rule: rules){
		if(rule->getTerms().size() == 0){
			epsilon1 = true;
		}
		for(auto term : rule->getTerms()){
			epsilon2 = true;
			int termId = _ast.termStore.query(term);
			if(termId != id){
				if(calculated[termId] == UNDER_PROCESS){
					processed = false;
					continue;
				}
				for(auto str: firstOf(termId)){
					if(epsilonAssumption){
						ifEpsilon.insert(str);
					}else{
						firsts[id].insert(str);
					}
				}
			}else{
				epsilonAssumption = !epsilon;
				continue;
			}
			if(firsts[id].find(EPSILON_STR) == firsts[id].end()){
				epsilon2 = false;
				break;
			}
			firsts[id].erase(EPSILON_STR);
		}
		epsilonAssumption = false;
		epsilon = epsilon | epsilon1 | epsilon2 ;
	}
	if(epsilon){
		firsts[id].insert(EPSILON_STR);
		for(auto str: ifEpsilon){
			firsts[id].insert(str);
		}
	}
	if(processed){
		calculated[id] = PROCESSED;
	} else {
		calculated[id] = NOT_PROCESSED;
	}
	ifEpsilon.clear();
	return firsts[id];
}
