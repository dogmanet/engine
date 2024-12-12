// var lexer = require("./lib/lex-unit");
var lexer = require("./lib/lex");
var fs = require('fs');
var ini = require('ini');
const { promisify } = require('util');
var ReadFile = promisify(fs.readFile);
var WriteFile = promisify(fs.writeFile);
var ReadDir = promisify(fs.readdir);
var Unlink = promisify(fs.unlink);

var g_entDesc = {};
var g_mCClasses = {};
var g_mEntClasses = {};
var g_mCBases = {};

g_mFieldTypes = {
	"DEFINE_FIELD_VECTOR": "float3",
	"DEFINE_FIELD_VECTOR4": "float4",
	"DEFINE_FIELD_STRING": "string",
	"DEFINE_FIELD_ANGLES": "angles",
	"DEFINE_FIELD_INT": "int",
	"DEFINE_FIELD_ENUM": null,
	"DEFINE_FIELD_FLOAT": "float",
	"DEFINE_FIELD_BOOL": "bool",
	"DEFINE_FIELD_ENTITY": null,
	"DEFINE_FIELD_PARENT": "entity"
};

g_mInputTypes = {
	"PDF_NONE": "none",
	"PDF_INT": "int",
	"PDF_FLOAT": "float",
	"PDF_VECTOR": "float3",
	"PDF_VECTOR4": "float4",
	"PDF_BOOL": "bool",
	"PDF_STRING": "string",
	"PDF_ANGLES": "angles",
};

var ProcessFile = async function(file){
	var data = await ReadFile(file, {encoding: 'utf-8'});
	console.log('Processing ', file);
	// console.time("tokenize");
	var tokenStream = lexer.tokenize(data);
	// console.timeEnd("tokenize");
	// console.log(tokenStream);
	var _curTok = null;
	var GetNextLex = function(){
		var tok = tokenStream.shift();
		_curTok = tok;
		
		if(tok && tok.tokenClass == 'COMMENT'){
			if(tok.lexeme[1] == '*'){
				tok.lexeme = tok.lexeme.substr(0, tok.lexeme.length - 2);
			}
			if(tok.lexeme[2] != '!'){
				return(GetNextLex());
			}
			if(tok.lexeme[3] == '<'){
				tok.tokenClass = 'COMMENT_BACK';
				tok.lexeme = tok.lexeme.substr(4);
			}
			else{
				tok.lexeme = tok.lexeme.substr(3);
			}
			// //!<
			// //!
			// /*!
		}
		
		return(tok);
	};
	var GetCurLex = function(){
		return(_curTok);
	};
	// console.log(tokenStream);
	
	var ParseRegisterParam = function(type){
		var iParen = 1;
		var pLex;
		var out = [];
		while(iParen > 0 && (pLex = GetNextLex()))
		{
			switch(pLex.tokenClass){
			case '(':
				++iParen;
				break;
			case ')':
				--iParen;
				break;
			}
			if(iParen > 0){
				out.push(pLex);
			}
		}
		
		if(type == 'REC_ICON'){
			out = out[0].lexeme;
		}
		
		return(out);
	};
	
	var CClass = function(){
		var out = {};
		out.m_vProps = [];
		out.m_vOuts = [];
		out.m_vIns = [];
		out.m_vFlags = [];
		return(out);
	};
	var CProp = function(){
		var out = {};
		out.szKeyname = '';
		out.szName = '';
		out.szType = '';
		out.szComment = '';
		out.m_vOpts = [];
		return(out);
	};
	var COption = function(){
		var out = {};
		out.szName = '';
		out.szComment = null;
		return(out);
	};
	var COut = function(){
		var out = {};
		out.szName = '';
		out.szComment = null;
		return(out);
	};
	var CIn = function(){
		var out = {};
		out.szName = '';
		out.szType = '';
		out.szComment = null;
		return(out);
	};
	var CFlag = function(){
		var out = {};
		out.szName = '';
		out.szComment = null;
		return(out);
	};
		
	var pLex, pCmt, pCO, pCP, pCin, pCout, pCF, pCP2, szClassName;
	
	var cclass = CClass();
	
	while((pLex = GetNextLex())){
		// console.log(pLex);
		if(pLex.tokenClass == 'COMMENT'){
			pCmt = pLex;
			pLex = GetNextLex();
			if(!pLex){
				break;
			}
			
			var len = '\\skydocent'.length;
			var pos = pCmt.lexeme.indexOf('\\skydocent');
			if(pos < 0){
				pos = pCmt.lexeme.indexOf('@entity');
				len = '@entity'.length;
			}
			if(pos >= 0){
				pos += len;
				var text = pCmt.lexeme.substr(pos);
				text = text.trim();
				pos = text.indexOf('\n');
				if(pos >= 0){
					var cls = text.substr(0, pos);
					text = text.substr(pos).trim();
					g_entDesc[cls] = text;
				}
			}
		}
		else{
			pCmt = null;
		}
		
		if(pLex.tokenClass == 'COMMENT_BACK'){
			if(pCO){
				pCO.szComment = pLex.lexeme;
			}
			else if(pCP){
				pCP.szComment = pLex.lexeme;
			}
			if(pCin){
				pCin.szComment = pLex.lexeme;
			}
			if(pCout){
				pCout.szComment = pLex.lexeme;
			}
			if(pCF){
				pCF.szComment = pLex.lexeme;
			}
		}
		if(pCO && pCP2){
			pCP2.m_vOpts.push(pCO);
			pCO = null;
		}
		if(pCP){
			cclass.m_vProps.push(pCP);
			pCP = null;
			// cclass.m_vProps.push(CProp());
			pCP2 = cclass.m_vProps[cclass.m_vProps.length - 1];
			// console.log('pCP2: ', pCP2);
		}
		if(pCin){
			cclass.m_vIns.push(pCin);
			pCin = null;
		}
		if(pCout){
			cclass.m_vOuts.push(pCout);
			pCin = null;
		}
		if(pCF){
			cclass.m_vFlags.push(pCF);
			pCin = null;
		}
		if(pLex.tokenClass == 'IDENTIFIER')
		{
			if(pLex.lexeme == "BEGIN_PROPTABLE" || pLex.lexeme == "BEGIN_PROPTABLE_NOBASE"){
				pLex = GetNextLex();
				if(pLex.tokenClass == '('){
					pLex = GetNextLex();
					if(pLex.tokenClass == 'IDENTIFIER'){
						szClassName = pLex.lexeme;
						pLex = GetNextLex();
						if(pLex.tokenClass == ')'){
							console.log("Parsing class %s...", szClassName);
							cclass = CClass();
						}
					}
				}
			}
			else if(pLex.lexeme == "END_PROPTABLE"){
				if(szClassName){
					console.log("Done");
					g_mCClasses[szClassName] = cclass;
					szClassName = null;

					pCP = null;
					pCO = null;
					pCin = null;
					pCout = null;
					pCF = null;
					pCP2 = null;
				}
				else{
					console.error("Unexpected END_PROPTABLE");
				}
				//cclass
			}
			if(pLex.lexeme == "REGISTER_ENTITY_NOLISTING" || pLex.lexeme == "REGISTER_ENTITY"){
				var isNolisting = pLex.lexeme == "REGISTER_ENTITY_NOLISTING";
				pLex = GetNextLex(); // (
				pLex = GetNextLex(); // class
				var szClassName = pLex.lexeme;

				pLex = GetNextLex(); // ,
				pLex = GetNextLex(); // name
				var szName = pLex.lexeme;
				
				var params = [];
				do{
					pLex = GetNextLex(); // )
					if(pLex.tokenClass == 'IDENTIFIER'){
						var lex2 = GetNextLex();
						if(lex2.tokenClass == '('){
							params.push({
								type: pLex.lexeme,
								data: ParseRegisterParam(pLex.lexeme)
							});
						}
						else{
							params.push({
								type: pLex.lexeme,
								data: null
							});
						}
					}
					// console.log(pLex);
				}
				while(pLex.tokenClass != ')');
				g_mEntClasses[szName] = {szClassName, isNolisting, params};
			}
			if(pLex.lexeme == "class"){
				pLex = GetNextLex(); // class
				var szClassName = pLex.lexeme;
				pLex = GetNextLex(); // :
				var reserved = ['public', 'private', 'protected'];
				if(pLex.tokenClass == ':'){
					var bases = [];
					do{
						pLex = GetNextLex(); // )
						if(pLex.tokenClass == 'IDENTIFIER' && reserved.indexOf(pLex.lexeme) < 0){
							bases.push(pLex.lexeme);
						}
					}
					while(pLex.tokenClass != '{');
					g_mCBases[szClassName] = bases;
				}
			}
			else if(szClassName){
				// console.log('>', pLex);
				if((pLex.lexeme in g_mFieldTypes) || (pLex.lexeme.substr(0, pLex.lexeme.length - 2) in g_mFieldTypes)){
					pCO = null;
					pCin = null;
					pCout = null;
					pCF = null;
					pCP2 = null;

					var isFn = false;
					if(!(pLex.lexeme in g_mFieldTypes)){
						pLex.lexeme = pLex.lexeme.substr(0, pLex.lexeme.length - 2);
						isFn = true;
					}

					pCP = CProp();
					//memset(pCP, 0, sizeof(*pCP));
					// pCP.szComment = null;
					pCP.szType = g_mFieldTypes[pLex.lexeme];

					pLex = GetNextLex(); // (
					if(!pCP.szType){
						pLex = GetNextLex(); // type
						pCP.szType = pLex.lexeme;
						pLex = GetNextLex(); // ,
					}
					pLex = GetNextLex(); // field
					pLex = GetNextLex(); // ,
					while(GetNextLex() && GetCurLex().tokenClass != ','){} // flags ,
					pLex = GetNextLex(); // name
					pCP.szKeyname = pLex.lexeme;
					pLex = GetNextLex(); // ,
					pLex = GetNextLex(); // edname
					pCP.szName = pLex.lexeme;
					pLex = GetNextLex(); // ,
					if(isFn){
						pLex = GetNextLex(); // fn
						pLex = GetNextLex(); // ,
					}
					pLex = GetNextLex(); // editor
					pLex = GetNextLex(); // )

					if(pCmt){
						pCP.szComment = pCmt.lexeme;
					}
				}
				else if(pLex.lexeme == "COMBO_OPTION"){
					pCO = null;
					pCin = null;
					pCout = null;
					pCF = null;
					// pCP2 = null;
					// console.log(pLex);
					// console.log(pCP);
					// console.log(pCP2);
					if(pCP2){
						pCO = COption();
						// memset(pCO, 0, sizeof(*pCO));

						pLex = GetNextLex(); // (
						pLex = GetNextLex(); // name
						pCO.szName = pLex.lexeme;

						pLex = GetNextLex(); // ,
						pLex = GetNextLex(); // value
						pLex = GetNextLex(); // )

						if(pCmt){
							pCO.szComment = pCmt.lexeme;
						}
					}
				}
				else if(pLex.lexeme == "DEFINE_OUTPUT"){
					pCP = null;
					pCO = null;
					pCin = null;
					pCF = null;
					pCP2 = null;

					pCout = COut();
					// memset(pCout, 0, sizeof(*pCout));

					pLex = GetNextLex(); // (
					pLex = GetNextLex(); // field
					pLex = GetNextLex(); // ,
					pLex = GetNextLex(); // name
					pLex = GetNextLex(); // ,
					pLex = GetNextLex(); // edname
					pCout.szName = pLex.lexeme;

					pLex = GetNextLex(); // )

					if(pCmt){
						pCout.szComment = pCmt.lexeme;
					}
				}
				else if(pLex.lexeme == "DEFINE_INPUT"){
					pCP = null;
					pCO = null;
					pCout = null;
					pCF = null;
					pCP2 = null;

					pCin = CIn();

					pLex = GetNextLex(); // (
					pLex = GetNextLex(); // field
					pLex = GetNextLex(); // ,
					pLex = GetNextLex(); // name
					pLex = GetNextLex(); // ,
					pLex = GetNextLex(); // edname
					pCin.szName = pLex.lexeme;

					pLex = GetNextLex(); // ,
					pLex = GetNextLex(); // type
					pCin.szType = pLex.lexeme;
					if(pLex.lexeme in g_mInputTypes)
					{
						pCin.szType = g_mInputTypes[pLex.lexeme];
					}

					pLex = GetNextLex(); // )

					if(pCmt){
						pCin.szComment = pCmt.lexeme;
					}
				}
				else if(pLex.lexeme == "DEFINE_FLAG")
				{
					pCO = null;
					pCin = null;
					pCout = null;
					pCP = null;
					pCP2 = null;
					pCF = CFlag();
					// memset(pCF, 0, sizeof(*pCF));

					pLex = GetNextLex(); // (
					pLex = GetNextLex(); // value
					pLex = GetNextLex(); // ,
					pLex = GetNextLex(); // name
					pCF.szName = pLex.lexeme;
					pLex = GetNextLex(); // )

					if(pCmt){
						pCF.szComment = pCmt.lexeme;
					}
				}
				else{
					pCP = null;
					pCO = null;
					pCin = null;
					pCout = null;
					pCF = null;
					pCP2 = null;
				}
			}
		}
	}
};

var HasPropsForClass = function(cls){
	return(cls in g_mCClasses && g_mCClasses[cls].m_vProps.length);
};
var HasFlagsForClass = function(cls){
	return(cls in g_mCClasses && g_mCClasses[cls].m_vFlags.length);
};
var HasInsForClass = function(cls){
	return(cls in g_mCClasses && g_mCClasses[cls].m_vIns.length);
};
var HasOutsForClass = function(cls){
	return(cls in g_mCClasses && g_mCClasses[cls].m_vOuts.length);
};
var GenPropsForClass = function(cls, fileData){
	var data = g_mCClasses[cls].m_vProps;
	data.sort(function(a, b){
		if(a.szName.length == 0 && b.szName.length != 0){
			return(1);
		}
		if(a.szName.length != 0 && b.szName.length == 0){
			return(-1);
		}
		var cmp = a.szName.localeCompare(b.szName);
		if(cmp == 0){
			cmp = a.szKeyname.localeCompare(b.szKeyname);
		}
		return(cmp);
	});
	
	var hasUneditableProps = false;
	for(var i = 0, l = data.length; i < l; ++i){
		var prop = data[i];
		// if(prop.szName != ''){
			if(prop.szName){
				fileData.push('#### '+prop.szName+' ['+prop.szType+']');
			}
			else{
				if(!hasUneditableProps){
					hasUneditableProps = true;
					fileData.push(':::note\n');
					fileData.push('Нижеперечисленные свойства редактируются только программно, либо через файлы `classes.ent` или `defaults.ent`.');
				}
				fileData.push('#### _'+prop.szKeyname+'_ ['+prop.szType+']');
			}
			var szComment = (prop.szComment || '').trim();
			if(szComment.length && ['.', ':', '!', '?'].indexOf(szComment[szComment.length - 1]) < 0){
				szComment += prop.m_vOpts.length ? ':' : '.';
			}
			fileData.push(szComment);
			fileData.push('');
			
			if(prop.m_vOpts.length){
				for(var j = 0, jl = prop.m_vOpts.length; j < jl; ++j){
					var opt = prop.m_vOpts[j];
					if(opt.szComment){
						fileData.push('- **'+opt.szName+'** - '+opt.szComment+(j == jl - 1 ? '.' : ';'));
					}
					else{
						fileData.push('- **'+opt.szName+'**'+(j == jl - 1 ? '.' : ';'));
					}
				}
			}
		// }
		// else{
			// console.log(prop);
		// }
	}
	
	if(hasUneditableProps){
		fileData.push(':::');
	}
};
var GenFlagsForClass = function(cls, fileData){
	var data = g_mCClasses[cls].m_vFlags;
	for(var i = 0, l = data.length; i < l; ++i){
		var flag = data[i];
		if(flag.szName != ''){
			fileData.push('#### '+flag.szName);
			fileData.push(flag.szComment);
			fileData.push('');
		}
	}
};
var GenInsForClass = function(cls, fileData){
	var data = g_mCClasses[cls].m_vIns;
	// console.log(data);
	for(var i = 0, l = data.length; i < l; ++i){
		var inp = data[i];
		if(inp.szName != ''){
			fileData.push('#### '+inp.szName+(inp.szType == 'none' ? '' : ' ['+inp.szType+']'));
			fileData.push((inp.szComment || '').trim());
			fileData.push('');
		}
	}
};
var GenOutsForClass = function(cls, fileData){
	var data = g_mCClasses[cls].m_vOuts;
	// console.log(data);
	for(var i = 0, l = data.length; i < l; ++i){
		var outp = data[i];
		if(outp.szName != ''){
			fileData.push('#### '+outp.szName);
			fileData.push((outp.szComment || '').trim());
			fileData.push('');
		}
	}
};

(async function(){
	var ignoredFiles = [
		'proptable.h',
		'EntityFactory.h'
	];
	
	// await ProcessFile("../source/game/BaseLight.h");
	// await ProcessFile("../source/game/1parsertest.cpp");
	var baseDir = "../source/game/";
	var dir = await ReadDir(baseDir);
	// console.log(dir);
	var calls = [];
	for(var i = 0, l = dir.length; i < l; ++i){
		var f = dir[i];
		// console.log(f.split('.').pop());
		if(['h', 'cpp'].indexOf(f.split('.').pop()) >= 0 && ignoredFiles.indexOf(f) < 0){
			calls.push(ProcessFile(baseDir+f));
		}
	}
	console.log('awaiting', calls.length, 'files');
	await Promise.all(calls);
	
	var iniData = await ReadFile('../build/gamesource/config/entities/classes.ent', {encoding: 'utf-8'});
	iniData = ini.parse(iniData);
	
	for(var entClass in iniData){
		var classDesc = iniData[entClass];
		
		while(!('base_class' in classDesc)){
			var found = false;
			for(var i in classDesc){
				classDesc = classDesc[i];
				entClass += '.'+i;
				found = true;
				break;
			}
			if(!found){
				break;
			}
		}
		
		if('@description' in classDesc){
			g_entDesc[entClass] = classDesc['@description'];
		}
		
		var baseEntClass = classDesc.base_class;
		if(baseEntClass in g_mEntClasses){
			g_mEntClasses[entClass] = {
				szClassName: g_mEntClasses[baseEntClass].szClassName,
				isNolisting: classDesc.show_in_listing == 0,
				params: []
			};
		}
		else{
			console.error('Unknown base class', baseEntClass, 'for class', entClass, 'defined in classes.ent');
		}
	}
	
	// console.log(iniData);
	// console.log('g_mEntClasses', g_mEntClasses);
	// return;
	
	// console.log('g_entDesc', g_entDesc);
	// console.log('g_mCClasses', g_mCClasses);
	// console.log('g_mEntClasses', g_mEntClasses);
	// console.log('g_mCBases', g_mCBases);
	
	var targetDir = 'docs/entities/';
	var dir = await ReadDir(targetDir);
	var calls = [];
	for(var i = 0, l = dir.length; i < l; ++i){
		var f = dir[i];
		if(f != '_category_.json'){
			calls.push(Unlink(targetDir+f));
		}
	}
	console.log('awaiting', calls.length, 'files');
	await Promise.all(calls);
	
	calls = [];
	for(var entClass in g_mEntClasses){
		var entClassDesc = g_mEntClasses[entClass];
		
		if(entClassDesc.isNolisting){
			continue;
		}
		
		var icon = null;
		
		for(var i = 0, l = entClassDesc.params.length; i < l; ++i){
			var param = entClassDesc.params[i];
			if(param.type == 'REC_ICON'){
				icon = param.data;
			}
		}
		
		
		var cClassName = entClassDesc.szClassName;
		var bases = [];
		var cCurClass = cClassName;
		while(cCurClass in g_mCBases){
			cCurClass = g_mCBases[cCurClass][0];
			bases.push(cCurClass);
		}
		
		var classes = bases.slice();
		classes.unshift(cClassName);
		
		var fileData = [];
		
		fileData.push('---');
		fileData.push('custom_edit_url: null');
		fileData.push('---');
		fileData.push('');
		fileData.push('# '+entClass);
		
		if(icon){
			// fileData.push('Icon: '+icon);
			// fileData.push('');
		}
		
		if(entClass in g_entDesc){
			fileData.push('## Описание\n');
			fileData.push(g_entDesc[entClass]);
			fileData.push('');
		}
		
		fileData.push('## Свойства\n');
		GenPropsForClass(cClassName, fileData);
		for(var i = 0, l = bases.length; i < l; ++i){
			if(HasPropsForClass(bases[i])){
				fileData.push('---');
				fileData.push('_Унаследовано от класса '+bases[i]+'_:\n');
				GenPropsForClass(bases[i], fileData);
			}
		}
		
		var found = false;
		for(var i = 0, l = classes.length; i < l; ++i){
			if(HasFlagsForClass(classes[i])){
				found = true;
				break;
			}
		}
		if(found){
			fileData.push('## Флаги\n');
			GenFlagsForClass(cClassName, fileData);
			for(var i = 0, l = bases.length; i < l; ++i){
				if(HasFlagsForClass(bases[i])){
					fileData.push('---');
					fileData.push('_Унаследовано от класса '+bases[i]+'_:\n');
					GenFlagsForClass(bases[i], fileData);
				}
			}
		}
		
		found = false;
		for(var i = 0, l = classes.length; i < l; ++i){
			if(HasInsForClass(classes[i])){
				found = true;
				break;
			}
		}
		if(found){
			fileData.push('## Входы\n');
			GenInsForClass(cClassName, fileData);
			for(var i = 0, l = bases.length; i < l; ++i){
				if(HasInsForClass(bases[i])){
					fileData.push('---');
					fileData.push('_Унаследовано от класса '+bases[i]+'_:\n');
					GenInsForClass(bases[i], fileData);
				}
			}
		}
		
		found = false;
		for(var i = 0, l = classes.length; i < l; ++i){
			if(HasOutsForClass(classes[i])){
				found = true;
				break;
			}
		}
		if(found){
			fileData.push('## Выходы\n');
			GenOutsForClass(cClassName, fileData);
			for(var i = 0, l = bases.length; i < l; ++i){
				if(HasOutsForClass(bases[i])){
					fileData.push('---');
					fileData.push('_Унаследовано от класса '+bases[i]+'_:\n');
					GenOutsForClass(bases[i], fileData);
				}
			}
		}
		// Выходы
		
		fileData.push('## Реализация\n');
		fileData.push('Представлен классом: `'+cClassName+'`.');
		fileData.push('');
		var basesText = ['`'+cClassName+'`'];
		for(var i = 0, l = bases.length; i < l; ++i){
			basesText.push('`'+bases[i]+'`');
		}
		fileData.push('Цепочка наследования: '+basesText.join(' > ')+'.');
		
		calls.push(WriteFile(targetDir+entClass+'.md', fileData.join('\n'), {encoding: 'utf-8'}));
	}
	console.log('awaiting', calls.length, 'files');
	await Promise.all(calls);
	
	// await ProcessFile("../source/game/BaseLight.h");
	// await ProcessFile("../source/game/BaseLight.cpp");
	// await ProcessFile("../source/game/LightPoint.cpp");
	// console.log(g_mCClasses.CBaseTool);
})()
