#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>

using namespace std;

class Mips{
	public:
		int pc;

			Mips(istream &fin, ostream &fout):
				_top(0), _reg{0, 9, 8, 7, 1, 2, 3, 4, 5, 6}, _mem{5, 9, 4, 8, 7}, _fin(fin), _fout(fout),  _wb(*this), _memio(*this), _exe(*this), _id(*this), _if(*this), memwb{{0}, 0}, exemem{{0}, 0}, idexe{{0}, 0, 0, 0, 0, 0, 0, "00000000000000000000000000000000"}, ifid{0, "00000000000000000000000000000000"} {
					string input;
					while(fin >> input){
						_tape[_top++] = input;
					}
					
				}

		void writeReg(int index, int r){
			_reg[index] = r;
		}

		void writeMem(int index, int r){
			if(index) _mem[index] = r;
		}

		int readReg(int index){
			return _reg[index];
		}

		int readMem(int index){
			return _mem[index];
		}

		string getInput(int index){
			if(index >= _top){
				return "00000000000000000000000000000000";
			}else{
				return _tape[index];
			}
		}

		void run(){
			string input;
			bool isRunning = true;
			int cc = 0;
			pc = 0;
			while(isRunning){
				_solveHazard();
				_wb.run();
				isRunning = _memio.run();
				isRunning = isRunning | _exe.run();
				isRunning = isRunning | _id.run();
				isRunning = isRunning | _if.run();
				_fout << "CC " << ++cc << ":" << endl << endl
					<< "Registers:" << endl;
				for(int i = 0; i < 10; i++){
					_fout << "$" << i << ": " << _reg[i] << endl;
				}
				_fout << endl << "Data memory:" << endl
					<< "0x00: " << _mem[0] << endl
					<< "0x04: " << _mem[1] << endl
					<< "0x08: " << _mem[2] << endl
					<< "0x0C: " << _mem[3] << endl
					<< "0x10: " << _mem[4] << endl
					<< endl;
				_fout << "IF/ID :" << endl
					<< "PC\t\t" << ifid.pc << endl
					<< "Instruction\t" << ifid.machineCode << endl
					<< endl;
				_fout << "ID/EX :" << endl
					<< "ReadData1\t" << idexe.a << endl
					<< "ReadData2\t" << idexe.b << endl
					<< "sign_ext\t" << idexe.sign << endl
					<< "Rs\t\t" << idexe.rs << endl
					<< "Rt\t\t" << idexe.rt << endl
					<< "Rd\t\t" << idexe.rd << endl;
				_fout << "Control signals\t";
				for(int i = 0; i < 9; i++){
					_fout << idexe.control[i];
				}
				_fout << endl << endl;
				_fout << "EX/MEM :" << endl
					<< "ALUout\t\t" << exemem.ALUOut << endl
					<< "WriteData\t" << exemem.writeData << endl
					<< "Rt/Rd\t\t" << exemem.dest << endl;
				_fout << "Control signals ";
				for(int i = 0; i < 5; i++){
					_fout << exemem.control[i];
				}
				_fout << endl << endl;
				_fout << "MEM/WB :" << endl
					<< "ReadData\t" << memwb.readOutData << endl
					<< "ALUout\t\t" << memwb.ALUOut << endl
					<< "Rt/Rd\t\t" << memwb.saveDest << endl;
				_fout << "Control signals ";
				for(int i = 0; i < 2; i++){
					_fout << memwb.control[i];
				}
				_fout << endl;
				_fout << "=================================================================" << endl;
			}
		}


		struct MemWb{
			int control[2];
			int readOutData;
			int ALUOut;
			int saveDest;
		} memwb;

		struct ExeMem{
			int control[5];
			int writeData;
			int ALUOut;
			int dest;
		} exemem;

		struct IdExe{
			int control[9];
			int a;
			int b;
			int sign;
			int rs;
			int rt;
			int rd;
			string op;
		} idexe;

		struct IfId{
			int pc;
			string machineCode;
		} ifid;

	private:
		string _tape[100];
		int _top;
		istream &_fin;
		ostream &_fout;
		int _reg[10];
		int _mem[5];

		void _solveHazard(){
			if(idexe.op == "bne" && idexe.a != idexe.b){
				ifid.machineCode = "00000000000000000000000000000000";
				pc = ifid.pc = idexe.sign * 4 + 4;
			}
			if(memwb.control[0] && memwb.saveDest && (exemem.dest != idexe.rs)){
				if(memwb.saveDest == idexe.rs) idexe.a = memwb.control[1] ? memwb.readOutData : memwb.ALUOut;
				if(memwb.saveDest == idexe.rt) idexe.b = memwb.control[1] ? memwb.readOutData : memwb.ALUOut;
			}
			if(exemem.control[3] && exemem.dest){
				if(exemem.dest == idexe.rs) idexe.a = exemem.ALUOut;
				if(exemem.dest == idexe.rt) idexe.b = exemem.ALUOut;
			}
			if(idexe.control[5] && ((idexe.rt == stoi(ifid.machineCode.substr(6, 5), nullptr, 2)) || (idexe.rt == stoi(ifid.machineCode.substr(11, 5), nullptr, 2)))){
				ifid.machineCode = "00000000000000000000000000000000";
				pc -= 4;
				ifid.pc -= 4;
			}
		}

		class WriteBack{
			public:
				WriteBack(Mips &outer): _parrent(outer){
				}

				bool run(){
					if(_parrent.memwb.control[0]){
						_parrent.writeReg(_parrent.memwb.saveDest, (_parrent.memwb.control[1] ? _parrent.memwb.readOutData : _parrent.memwb.ALUOut));
						return true;
					}
					return false;
				}

			private:
				Mips &_parrent;
		} _wb;

		class Mem{
			public:
				Mem(Mips &outer): _parrent(outer){
				}

				bool run(){
					_run = false;
					_readData = 0;
					if(_parrent.exemem.control[1]){
						_run = true;
						_readData = _parrent.readMem(_parrent.exemem.ALUOut / 4);
					}
					if(_parrent.exemem.control[2]){
						_run = true;
						_parrent.writeMem(_parrent.exemem.ALUOut / 4, _parrent.exemem.writeData);
					}
					if(_parrent.exemem.control[3]) _run = true;
					_pipe();
					return _run;
				}

			private:
				Mips &_parrent;
				bool _run;
				int _readData;

				void _pipe(){
					_parrent.memwb.readOutData = _readData;
					_parrent.memwb.ALUOut = _parrent.exemem.ALUOut;
					_parrent.memwb.saveDest = _parrent.exemem.dest;
					for(int i = 0; i < 2; i++){
						_parrent.memwb.control[i] = _parrent.exemem.control[i + 3];
					}
				}

		} _memio;

		class Exe{
			public:
				Exe(Mips &outer): _parrent(outer){
				}

				bool run(){
					_run = true;
					_ALUout = 0;
					_dest = 0;
					string op = _parrent.idexe.op;
					if(op == "sll"){
						_run = false;
					}else if(op == "lw" || op == "sw" || op == "addi"){
						_ALUout = _parrent.idexe.a + _parrent.idexe.sign;
						_dest = _parrent.idexe.rt;
					}else if(op == "add"){
						_ALUout = _parrent.idexe.a + _parrent.idexe.b;
						_dest = _parrent.idexe.rd;
					}else if(op == "sub"){
						_ALUout = _parrent.idexe.a - _parrent.idexe.b;
						_dest = _parrent.idexe.rd;
					}else if(op == "and"){
						_ALUout = _parrent.idexe.a & _parrent.idexe.b;
						_dest = _parrent.idexe.rd;
					}else if(op == "andi"){
						_ALUout = _parrent.idexe.a & _parrent.idexe.sign;
						_dest = _parrent.idexe.rt;
					}else if(op == "or"){
						_ALUout = _parrent.idexe.a | _parrent.idexe.b;
						_dest = _parrent.idexe.rd;
					}else if(op == "slt"){
						_ALUout = _parrent.idexe.a < _parrent.idexe.b ? 1 : 0;
						_dest = _parrent.idexe.rd;
					}else if(op == "bne"){
						_ALUout = _parrent.idexe.a - _parrent.idexe.b;
						_dest = _parrent.idexe.rt;
					}
					_pipe();
					return _run;
				}

			private:
				Mips &_parrent;
				bool _run;
				int _ALUout;
				int _dest;

				void _pipe(){
					_parrent.exemem.writeData = _parrent.idexe.b;
					_parrent.exemem.dest = _dest;
					_parrent.exemem.ALUOut = _ALUout;
					for(int i = 0; i < 5; i++){
						_parrent.exemem.control[i] = _parrent.idexe.control[i + 4];
					}
				}
		} _exe;

		class Id{
			public:
				Id(Mips &outer): _parrent(outer){
				}

				bool run(){
					char *c;
					int parsed = stoi(_parrent.ifid.machineCode.substr(0, 6), nullptr, 2);
					_run = true;
					_rs = stoi(_parrent.ifid.machineCode.substr(6, 5), nullptr, 2);
					_rt = stoi(_parrent.ifid.machineCode.substr(11, 5), nullptr, 2);
					_rd = stoi(_parrent.ifid.machineCode.substr(16, 5), nullptr, 2);
					_sign = stoi(_parrent.ifid.machineCode.substr(16, 16), nullptr, 2);
					_func = stoi(_parrent.ifid.machineCode.substr(26, 6), nullptr, 2);
					if(_parrent.ifid.machineCode[16] == 1){
						_sign = -(~_sign + 1);
					}
					_a = _parrent.readReg(_rs);
					_b = _parrent.readReg(_rt);
					for(int i = 0; i < 9; i++){
						_control[i] = 0;
					}

					if(parsed == 0 && _func == 0){
						_run = false;
						_op = "sll";
					}else if(parsed == 0 && _func == 32){
						_control[0] = _control[7] = _control[1] = 1;
						_op = "add";
					}else if(parsed == 0 && _func == 34){
						_control[0] = _control[7] = _control[1] = 1;
						_op = "sub";
					}else if(parsed == 0 && _func == 36){
						_control[0] = _control[7] = _control[1] = 1;
						_op = "and";
					}else if(parsed == 0 && _func == 37){
						_control[0] = _control[7] = _control[1] = 1;
						_op = "or";
					}else if(parsed == 0 && _func == 42){
						_control[0] = _control[7] = _control[1] = 1;
						_op = "slt";
					}else if(parsed == 5){
						_control[2] = _control[4] = 1;
						_op = "bne";
					}else if(parsed == 35){
						_control[3] = _control[5] = _control[7] = _control[8] = 1;
						_op = "lw";
					}else if(parsed == 43){
						_control[3] = _control[6] = 1;
						_op = "sw";
					}else if(parsed == 8){
						_control[3] = _control[7] = 1;
						_op = "addi";
					}else if(parsed == 12){
						_control[1] = _control[2] = _control[3] = _control[7] = 1;
						_op = "andi";
					}
					_pipe();
					return _run;
				}
				
			private:
				Mips &_parrent;
				int _a;
				int _b;
				int _sign;
				int _rs;
				int _rt;
				int _rd;
				int _func;
				int _control[9];
				string _op;
				bool _run;

				void _pipe(){
					_parrent.idexe.a = _a;
					_parrent.idexe.b = _b;
					_parrent.idexe.sign = _sign;
					_parrent.idexe.rs = _rs;
					_parrent.idexe.rd = _rd;
					_parrent.idexe.rt = _rt;
					_parrent.idexe.op = _op;
					for(int i = 0; i < 9; i++){
						_parrent.idexe.control[i] = _control[i];
					}
				}
		} _id;

		class If{
			public:
				If(Mips &outer): _parrent(outer){
				}

				bool run(){
					_run = true;
					string input = _parrent.getInput(_parrent.pc / 4);
					if(input == "00000000000000000000000000000000"){
						_run = false;
					}
					_parrent.pc += 4;
					_parrent.ifid.pc = _parrent.pc;
					_parrent.ifid.machineCode = input;
					return _run;
				}
			private:
				Mips &_parrent;
				bool _run;
		} _if;
};

int main(){
	ifstream fin;
	ofstream fout;
	//1
	fin.open("General.txt", ios::in);
	fout.open("genResult.txt", ios::out);
	Mips mips_1(fin, fout);
	mips_1.run();
	fin.close();
	fout.close();
	//2
	fin.open("Datahazard.txt", ios::in);
	fout.open("dataResult.txt");
	Mips mips_2(fin, fout);
	mips_2.run();
	fin.close();
	fout.close();
	//3
	fin.open("Lwhazard.txt", ios::in);
	fout.open("loadResult.txt");
	Mips mips_3(fin, fout);
	mips_3.run();
	fin.close();
	fout.close();
	//4
	fin.open("Branchhazard.txt", ios::in);
	fout.open("branchResult.txt");
	Mips mips_4(fin, fout);
	mips_4.run();
	fin.close();
	fout.close();
	return 0;
}
