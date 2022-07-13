#include "Player.hh"
#include <set>
#include <queue>
#include <map>


/**
 * Write the name of your player and save this file
 * with the same name and .cc extension.
 */
#define PLAYER_NAME Cristian


struct PLAYER_NAME : public Player {

	/**
	 * Factory: returns a new instance of this class.
	 * Do not modify this function.
	 */
	static Player* factory ()
	{
		return new PLAYER_NAME;
	}

	/**
	 * Types and attributes for your player can be defined here.
	 */
	//ATRIBUTS
	int queen;
	vector<int> my_worker_ids;
	vector<int> my_soldier_ids;
	map<int,Dir> dirs = {{0,Up},{1,Down},{2,Left},{3,Right}};
	vector<vector<bool>> takenPos;
	map<int,vector<Dir>> ant_nut;
	map<int,Pos> pos_obj;
	map<int,int>idxs;
	map<int,vector<vector<int>>> moves;
	set<Pos>presentNutrients;
	vector<Pos>nutrients;
	set<int>enemies;
	set<int>allAnts;
	pair<int,Pos>candidate;
	bool near = false;
	map<int,bool>needUpdate;
	queue<int> updateSit;

	//METODES
	bool nutrientsIncoming(Pos p)
	{
		set<Pos>positions({p+Up,p+Up+Right,p+Down,p+Down+Left,p+Left,p+Left+Up,p+Right,p+Right+Down});
		for(const Pos& cand: positions) if(pos_ok(cand)) {
				int id = cell(cand).id;
				if(id!=-1 and allAnts.find(id)!=allAnts.end() and ant(id).bonus!=None) return true;
			}
		return false;
	}

	vector<bool>& bonusNearby(Pos p,vector<bool>& ret)
	{
		vector<Pos>positions({p+Up,p+Up+Right,p+Down,p+Down+Left,p+Left,p+Left+Up,p+Right,p+Right+Down});
		int idx=0;
		for(int i=0; i<positions.size(); ++i) {
			Pos p = positions[i];
			if(pos_ok(p) and cell(p).bonus!=None) ret[idx]=true;
			if((i+1)%2==0) ++idx;
		}
		return ret;
	}

	vector<bool>& dangerNearby(Pos p, vector<bool>& ret)
	{
		vector<Pos>positions({p+Up,p+Up+Left,p+Up+Right,p+Up+Up,p+Down,p+Down+Left,p+Down+Right,p+Down+Down,p+Left,p+Left+Up,
		                      p+Left+Down,p+Left+Left,p+Right,p+Right+Down,p+Right+Up,p+Right+Right
		                     });
		int idx=0;
		for(int i=0; i<positions.size(); ++i) {
			Pos p = positions[i];
			if(pos_ok(p)) {
				Cell c = cell(p);
				if(c.id!=-1 and ant(c.id).type==Queen) ret[idx]=true;
			}
			if((i+1)%4==0) ++idx;
		}
		return ret;
	}

	vector<bool>& objectiveSoldier(Pos p, vector<bool>& ret)
	{
		vector<Pos>positions({p+Up,p+Up+Up,p+Up+Right,p+Down,p+Down+Down,p+Down+Left,p+Left,p+Left+Left,p+Left+Up,p+Right,p+Right+Right,p+Right+Down});
		int idx=0;
		for(int i=0; i<positions.size(); ++i) {
			Pos p = positions[i];
			if(pos_ok(p)) {
				Cell c = cell(p);
				if(c.id!=-1 and allAnts.find(c.id)==allAnts.end() and ant(c.id).type==Worker) ret[idx]=true;
			}
			if((i+1)%3==0) ++idx;
		}
		return ret;
	}

	vector<bool>& threatsNearby(Pos p, vector<bool>& ret)
	{
		vector<Pos>positions({p+Up,p+Up+Up,p+Up+Right,p+Up+Left,
		                      p+Down,p+Down+Down,p+Down+Left,p+Down+Right,
		                      p+Left,p+Left+Left,p+Left+Up,p+Left+Down,
		                      p+Right,p+Right+Right,p+Right+Down,p+Right+Up
		                     });
		int idx=0;
		for(int i=0; i<positions.size(); ++i) {
			Pos p = positions[i];
			if(pos_ok(p)) {
				Cell c = cell(p);
				if(c.id!=-1 and allAnts.find(c.id)==allAnts.end()) ret[idx]=true;
			}
			if((i+1)%4==0) ++idx;
		}
		return ret;
	}

	bool movable(Pos p_ant)
	{
		Pos up = p_ant + Up, down= p_ant + Down, left= p_ant + Left, right= p_ant + Right;
		set<Pos> positions({up,down,left,right});
		for(Pos p: positions) if(pos_ok(p) and not takenPos[p.i][p.j] and cell(p).type!=Water) return true;
		return false;
	}

	bool antNearQueen(Pos antpos)
	{
		Pos up = antpos + Up, down= antpos + Down, left= antpos + Left, right= antpos + Right;
		set<Pos> positions;
		positions.insert({up,down,left,right,up+Left,up+Right,down+Left,down+Right});
		return positions.find(ant(queen).pos)!=positions.end();
	}

	pair<bool,Pos> posNearQueen()
	{
		Pos p = ant(queen).pos;
		Pos up = p + Up, down= p + Down, left= p + Left, right= p + Right;
		set<Pos> positions;
		positions.insert({up,down,left,right,up+Left,up+Right,down+Left,down+Right});
		for(const Pos& pos: positions) if(pos_ok(pos) and not takenPos[pos.i][pos.j] and cell(pos).bonus==None) return {true,pos};
		return {false,Pos(0,0)};
	}

	bool dfs(int id, Pos p, vector<vector<bool>>& table)
	{
		if(p==pos_obj[id]) return true;
		table[p.i][p.j]=true;
		Pos aux=pos_obj[id];
		if(moves[id][p.i][p.j]>=moves[id][aux.i][aux.j]) return false;
		Dir d;
		vector<int>dir = random_permutation(4);
		for(const int& i: dir) {
			d = Dir(i);
			aux = p+d;
			if(pos_ok(aux) and not table[aux.i][aux.j] and moves[id][aux.i][aux.j]==(moves[id][p.i][p.j]+1)) {
				ant_nut[id].push_back(d);
				if(dfs(id,aux,table)) return true;
				ant_nut[id].pop_back();
			}
		}
		return false;
	}

	void standardDijkstra(int id, Pos p, int dist)
	{
		if(pos_ok(p) and cell(p).type!=Water and moves[id][p.i][p.j]>dist) {
			moves[id][p.i][p.j]=dist;
			standardDijkstra(id,p+Up,dist+1);
			standardDijkstra(id,p+Down,dist+1);
			standardDijkstra(id,p+Left,dist+1);
			standardDijkstra(id,p+Right,dist+1);
		}
	}

	void dijkstraWorker(int id, Pos p, int dist)
	{
		if(pos_ok(p) and cell(p).type!=Water and moves[id][p.i][p.j]>dist) {
			moves[id][p.i][p.j]=dist;
			if(cell(p).bonus!=None and candidate.first>dist and presentNutrients.find(p)==presentNutrients.end() and not antNearQueen(p)) candidate= {dist,p};
			dijkstraWorker(id,p+Up,dist+1);
			dijkstraWorker(id,p+Down,dist+1);
			dijkstraWorker(id,p+Left,dist+1);
			dijkstraWorker(id,p+Right,dist+1);
		}
	}

	void dijkstraSoldier(int id, Pos p, int dist)
	{
		if(pos_ok(p) and cell(p).type!=Water and moves[id][p.i][p.j]>dist) {
			moves[id][p.i][p.j]=dist;
			Cell c = cell(p);
			if(c.id!=-1 and allAnts.find(c.id)==allAnts.end() and candidate.first>dist and ant(c.id).type==Worker and enemies.find(c.id)==enemies.end()) candidate= {dist,p};
			dijkstraSoldier(id,p+Up,dist+1);
			dijkstraSoldier(id,p+Down,dist+1);
			dijkstraSoldier(id,p+Left,dist+1);
			dijkstraSoldier(id,p+Right,dist+1);
		}
	}

	void setObjectiveWorker(int id)
	{
		ant_nut[id].clear();
		Pos p = ant(id).pos;
		moves[id]=vector<vector<int>>(board_rows(),vector<int>(board_cols(),625));
		candidate= {625,Pos(0,0)};
		if(ant(id).bonus!=None) {
			standardDijkstra(id,p,0);
			pair<bool,Pos>result = posNearQueen();
			if(result.first) pos_obj[id]=result.second;
			else return;
			candidate.second=result.second;
		} else if(pos_obj.find(id)==pos_obj.end() or pos_obj[id]==p or ant(id).bonus==None) {
			dijkstraWorker(id,p,0);
			pos_obj[id] = candidate.second;
			presentNutrients.insert(candidate.second);
		}
		if(candidate.second!=Pos(0,0)) {
			vector<vector<bool>>table(board_rows(),vector<bool>(board_cols(),false));
			dfs(id,p,table);
		}
		idxs[id]=0;
	}

	void recalculateTrajectory(int id)
	{
		Ant a = ant(id);
		ant_nut[id].clear();
		Pos p = a.pos;
		moves[id]=vector<vector<int>>(board_rows(),vector<int>(board_cols(),625));
		vector<vector<bool>>table(board_rows(),vector<bool>(board_cols(),false));
		standardDijkstra(id,p,0);
		if(a.type==Worker and a.bonus!=None) pos_obj[id]=ant(queen).pos;
		dfs(id,p,table);
		idxs[id]=0;
	}

	void setObjectiveSoldier(int id)
	{
		Pos p = ant(id).pos;
		if(pos_obj.find(id)==pos_obj.end() or pos_obj[id]==p) {
			ant_nut[id].clear();
			moves[id]=vector<vector<int>>(board_rows(),vector<int>(board_cols(),625));
			candidate= {625,Pos(0,0)};
			dijkstraSoldier(id,p,0);
			pos_obj[id] = candidate.second;
			enemies.insert(cell(pos_obj[id]).id);
			if(candidate.second!=Pos(0,0)) {
				vector<vector<bool>>table(board_rows(),vector<bool>(board_cols(),false));
				dfs(id,p,table);
			}
			idxs[id]=0;
		}
	}

	void resetObjectiveWorker(int id)
	{
		ant_nut[id].clear();
		Pos p = ant(id).pos;
		moves[id]=vector<vector<int>>(board_rows(),vector<int>(board_cols(),625));
		candidate= {625,Pos(0,0)};
		dijkstraWorker(id,p,0);
		pos_obj[id] = candidate.second;
		presentNutrients.insert(candidate.second);
		vector<vector<bool>>table(takenPos);
		if(candidate.second!=Pos(0,0)) dfs(id,p,table);
		idxs[id]=0;
	}

	void randomMovement(int id, vector<bool>& accessPos)
	{
		vector<int>perms = random_permutation(4);
		Pos antpos = ant(id).pos, p;
		Dir d;
		for(const int& i: perms) {
			d=dirs[i];
			p=antpos+d;
			if(accessPos[i]) {
				move(id,d);
				takenPos[p.i][p.j]=true;
				return;
			}
		}
	}

	void moveWorker(int id)
	{
		Ant a = ant(id);
		Pos antpos = a.pos, p;
		if(antpos==pos_obj[id] and a.bonus==None) {
			take(id);
			presentNutrients.erase(antpos);
			moves.erase(id);
			return;
		}
		if((antNearQueen(antpos) and a.bonus!=None and cell(antpos).bonus==None) or a.life==1) {
			near=true;
			leave(id);
			presentNutrients.insert(antpos);
			pos_obj.erase(id);
			moves.erase(id);
			return;
		}
		vector<bool> accessPos(4,false), threats(4,false);
		threats = threatsNearby(antpos,threats);
		Pos up = antpos + Up, down= antpos + Down, left= antpos + Left, right= antpos + Right;
		accessPos[0]=(pos_ok(up) and cell(up).type != Water and not takenPos[up.i][up.j] and !threats[0]);
		accessPos[1]=(pos_ok(down) and cell(down).type != Water and not takenPos[down.i][down.j] and !threats[1]);
		accessPos[2]=(pos_ok(left) and cell(left).type != Water and not takenPos[left.i][left.j] and !threats[2]);
		accessPos[3]=(pos_ok(right) and cell(right).type != Water and not takenPos[right.i][right.j] and !threats[3]);
		if(threats[0] or threats[1] or threats[2] or threats[3]) {
			Dir d;
			if(accessPos[0]) d=Up;
			else if(accessPos[3]) d=Right;
			else if(accessPos[1]) d=Down;
			else if(accessPos[2]) d=Left;
			else return;
			move(id,d);
			antpos+=d;
			takenPos[antpos.i][antpos.j]=true;
			idxs[id]=-1;
			return;
		}
		if((idxs[id]==-1 or a.bonus!=None or pos_obj[id]==antpos or (pos_obj.find(id)!=pos_obj.end() and ant_nut[id].size()==0)) and round()%5==0) recalculateTrajectory(id);
		else if(pos_obj.find(id)==pos_obj.end()) setObjectiveWorker(id);
		if(idxs[id]<ant_nut[id].size()) {
			Dir d = ant_nut[id][idxs[id]];
			p=antpos+d;
			if(pos_ok(p) and not takenPos[p.i][p.j] and cell(p).type!=Water) {
				move(id,d);
				takenPos[p.i][p.j]=true;
				++idxs[id];
				return;
			} else {
				randomMovement(id,accessPos);
				idxs[id]=-1;
			}
		}
	}

	void moveSoldier(int id)
	{
		Pos antpos = ant(id).pos, p;
		vector<bool> accessPos(4,false), objectives(4,false);
		objectives=objectiveSoldier(antpos,objectives);
		Pos up = antpos + Up, down= antpos + Down, left= antpos + Left, right= antpos + Right;
		accessPos[0]=(pos_ok(up) and cell(up).type != Water and not takenPos[up.i][up.j] and cell(up).id==-1);
		accessPos[1]=(pos_ok(down) and cell(down).type != Water and not takenPos[down.i][down.j] and cell(down).id==-1);
		accessPos[2]=(pos_ok(left) and cell(left).type != Water and not takenPos[left.i][left.j] and cell(left).id==-1);
		accessPos[3]=(pos_ok(right) and cell(right).type != Water and not takenPos[right.i][right.j] and cell(right).id==-1);
		p=antpos;
		if(objectives[0] or objectives[1] or objectives[2] or objectives[3]) {
			Dir d;
			if(accessPos[0] and objectives[0]) d=Up;
			else if(accessPos[3] and objectives[3]) d=Right;
			else if(accessPos[1] and objectives[1]) d=Down;
			else if(accessPos[2] and objectives[2]) d=Left;
			else return;
			move(id,d);
			p+=d;
			takenPos[p.i][p.j]=true;
			idxs[id]=-1;
			return;
		}
		if((idxs[id]==-1 or pos_obj[id]==antpos or (pos_obj.find(id)!=pos_obj.end() and ant_nut[id].size()==0))) recalculateTrajectory(id);
		if(idxs[id]>=0 and idxs[id]<ant_nut[id].size()) {
			Dir d = ant_nut[id][idxs[id]];
			p=antpos+d;
			if(pos_ok(p) and not takenPos[p.i][p.j]) {
				move(id,d);
				takenPos[p.i][p.j]=true;
				++idxs[id];
				if(p==pos_obj[id]) setObjectiveSoldier(id);
			} else {
				randomMovement(id,accessPos);
				idxs[id]=-1;
			}
		}
	}

	void moveQueen()
	{
		Ant q = ant(queen);
		if(q.id==-1) return;
		Pos antpos = q.pos, p;
		vector<bool>accessPos(4,false), bonus(4,false);
		bonus=bonusNearby(antpos,bonus);
		Pos up = antpos + Up, down= antpos + Down, left= antpos + Left, right= antpos + Right;
		accessPos[0]=(pos_ok(up) and cell(up).type != Water and not takenPos[up.i][up.j]);
		accessPos[1]=(pos_ok(down) and cell(down).type != Water and not takenPos[down.i][down.j]);
		accessPos[2]=(pos_ok(left) and cell(left).type != Water and not takenPos[left.i][left.j]);
		accessPos[3]=(pos_ok(right) and cell(right).type != Water and not takenPos[right.i][right.j]);
		p=antpos;
		if((accessPos[0] and bonus[0]) or (accessPos[1] and bonus[1]) or (accessPos[2] and bonus[2]) or (accessPos[3] and bonus[3])) {
			Dir d;
			if(accessPos[0] and bonus[0]) d=Up;
			else if(accessPos[3] and bonus[3]) d=Right;
			else if(accessPos[1] and bonus[1]) d=Down;
			else if(accessPos[2] and bonus[2]) d=Left;
			else return;
			move(queen,d);
			p+=d;
			takenPos[p.i][p.j]=true;
			idxs[queen]=-1;
			return;
		}
		if(q.reserve[0]>0 and q.reserve[1]>0 and q.reserve[2]>0) {
			AntType type;
			for(int i=0; i<4; i++) if(accessPos[i]) {
					if (random(0, 1) and q.reserve[0]>=soldier_carbo() and q.reserve[1]>=soldier_prote() and q.reserve[2]>=soldier_lipid()) type=Soldier;
					else type = Worker;
					lay(queen,dirs[i],type);
					return;
				}
		}
		if(idxs[queen]==-1 or (pos_obj.find(queen)!=pos_obj.end() and ant_nut[queen].size()==0)) recalculateTrajectory(queen);
		if(idxs[queen]>=0 and idxs[queen]<ant_nut[queen].size()) {
			Dir d = ant_nut[queen][idxs[queen]];
			p=antpos+d;
			if(pos_ok(p) and not takenPos[p.i][p.j]) {
				move(queen,d);
				takenPos[p.i][p.j]=true;
				++idxs[queen];
				return;
			} else {
				randomMovement(queen,accessPos);
				idxs[queen]=-1;
			}
		}
	}

	/**
	 * Play method, invoked once per each round.
	 */
	virtual void play ()
	{
		if(round()>0 and queens(me()).size()>0) {
			if((workers(me()).size()!=my_worker_ids.size() or soldiers(me()).size()!=my_soldier_ids.size())) {
				allAnts.clear();
				queen=ant(queens(me())[0]).id;
				allAnts.insert(queen);
				my_worker_ids = workers(me());
				reverse(my_worker_ids.begin(),my_worker_ids.end());
				allAnts.insert(my_worker_ids.begin(),my_worker_ids.end());
				my_soldier_ids=soldiers(me());
				allAnts.insert(my_soldier_ids.begin(),my_soldier_ids.end());
			}
			takenPos= vector<vector<bool>>(board_rows(),vector<bool>(board_cols(),false));
			for(const int& id: allAnts) {
				Pos p = ant(id).pos;
				takenPos[p.i][p.j]=true;
			}
			if(round()!=1 and round()%bonus_period()==1) {
				presentNutrients.clear();
				for(const int& id: my_worker_ids) {
					if(ant(id).bonus==None or (pos_obj.find(id)!=pos_obj.end() and pos_obj[id]==Pos(0,0))) resetObjectiveWorker(id);
					moveWorker(id);
				}
				resetObjectiveWorker(queen);
			} else {
				for(const int& id: my_worker_ids) {
					if(moves.find(id)==moves.end()) setObjectiveWorker(id);
					if(movable(ant(id).pos)) moveWorker(id);
				}
			}
			for(const int& id: my_soldier_ids) {
				Pos p = ant(id).pos;
				if(moves.find(id)==moves.end() or (pos_obj.find(id)!=pos_obj.end() and pos_obj[id]==p)) setObjectiveSoldier(id);
				if(movable(p)) moveSoldier(id);
			}
			if(round()%queen_period()==0 and queens(me()).size()>0) {
				Pos p = ant(queen).pos;
				if(moves.find(queen)==moves.end() or (pos_obj.find(queen)!=pos_obj.end() and p==pos_obj[queen])) setObjectiveWorker(queen);
				if(!nutrientsIncoming(p) and movable(ant(queen).pos)) moveQueen();
			}
		}
	}
};

/**
 * Do not modify the following line.
 */
RegisterPlayer(PLAYER_NAME);
