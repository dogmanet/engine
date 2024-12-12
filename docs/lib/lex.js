var tokenize = function(streamOfText){
	streamOfText += '\n';
	var streamOfTokens = [];
	
	var cur = 0;
	
	var PushTok = function(lexeme, tokenClass){
		// console.log(lexeme);
		streamOfTokens.push({
			lexeme: lexeme,
			tokenClass: tokenClass ? tokenClass : lexeme
		});
	};
	
	var ReadOneline = function(){
		var start = cur;
		var len = 0;
		while(streamOfText[cur] != '\n' && streamOfText[cur] != '\r'){
			++len;
			++cur;
		}
		var str = streamOfText.substr(start, len);
		// memcpy(str, start, sizeof(char)* len);
		// str[len] = 0;

		if(streamOfText[start + 2] == '!' || streamOfText[start + 2] == '*'){
			// PushTok(start[3] == '<' ? LTYPE_COMMENT_ONELINE_BACK : LTYPE_COMMENT_ONELINE, str + (start[3] == '<' ? 4 : 3));
			PushTok(str, 'COMMENT');
		}
		return(cur - 1);
	};
	
	var ReadMultiline = function(){
		var start = cur;
		var len = 0;
		while(streamOfText[cur] && streamOfText[cur+1] && !(streamOfText[cur] == '*' && streamOfText[cur + 1] == '/')){
			++len;
			++cur;
		}
		var str = streamOfText.substr(start, len + 2);
		
		if(streamOfText[start + 2] == '!' || streamOfText[start + 2] == '*'){
			// PushTok(str.substr(3), 'COMMENT');
			PushTok(str, 'COMMENT');
		}
		++cur;
		// return(cur + 1);
	};
	
	var ReadInt = function(){
		var start = cur;
		var len = 0;
		while(streamOfText[cur].charCodeAt(0) >= '0'.charCodeAt(0) && streamOfText[cur].charCodeAt(0) <= '9'.charCodeAt(0)){
			++len;
			++cur;
		}
		PushTok(streamOfText.substr(start, len), 'INT');
		--cur;
	};
	
	var ReadSym = function(){
		var start = cur;
		var len = 0;
		var ccode;
		while((ccode = streamOfText[cur].charCodeAt(0))
			&& ((ccode >= '0'.charCodeAt(0) && ccode <= '9'.charCodeAt(0))
			|| (ccode >= 'a'.charCodeAt(0) && ccode <= 'z'.charCodeAt(0))
			|| (ccode >= 'A'.charCodeAt(0) && ccode <= 'Z'.charCodeAt(0))
			|| (ccode == '_'.charCodeAt(0)))
		){
			++len;
			++cur;
		}
		var str = streamOfText.substr(start, len);
		
		var found = false;
		/*for(var i = 0, l = sizeof(g_pszKeyWords) / sizeof(g_pszKeyWords[0]); i < l; ++i)
		{
			if(!strcmp(g_pszKeyWords[i], str))
			{
				PushTok(LTYPE_KW, str);
				found = true;
				break;
			}
		}*/
		if(!found){
			PushTok(str, 'IDENTIFIER');
		}
		--cur;
	};
	
	var ReadSym2 = function(){
		++cur;
		var start = cur;
		var len = 0;
		while(streamOfText[cur] != '"'){
			++len;
			++cur;
		}
		
		var str = [];
		var szSrc = start;
		for(var i = 0; i < len; ++i){
			if(streamOfText[szSrc] == '\\'){
				++szSrc;
				switch(streamOfText[szSrc]){
				case '"':
				case '\'':
				case '\\':
				case '/':
					str.push(streamOfText[szSrc]);
					break;
				case 'b':
					str.push('\b');
					break;
				case 'f':
					str.push('\f');
					break;
				case 'n':
					str.push('\n');
					break;
				case 'r':
					str.push('\r');
					break;
				case 't':
					str.push('\t');
					break;
				}
			}
			else{
				str.push(streamOfText[szSrc]);
			}
			++szSrc;

		}
		//memcpy(str, start, sizeof(char)* len);
		PushTok(str.join(''), 'CONSTANT');
		// return(cur);
	};

	while(streamOfText[cur]){
		// console.log(cur, streamOfText[cur]);
		switch(streamOfText[cur]){
		case ' ':
		case '\t':
		case '\n':
		case '\r':
			// Skip it
			break;
		case '(':
			PushTok("(");
			break;
		case ')':
			PushTok(")");
			break;
		case '{':
			PushTok("{");
			break;
		case '}':
			PushTok("}");
			break;
		case ':':
			PushTok(":");
			break;

		case '/':
			if(streamOfText[cur + 1] == '/')
			{
				ReadOneline();
			}
			else if(streamOfText[cur + 1] == '*')
			{
				ReadMultiline();
			}
			else
			{
				PushTok("/");
			}
			break;
		case ',':
			PushTok(",");
			break;
		case '"':
			ReadSym2();
			break;
		default:
			var ccode = streamOfText[cur].charCodeAt(0);
			if(ccode >= '0'.charCodeAt(0) && ccode <= '9'.charCodeAt(0)){
				ReadInt();
				break;
			}
			if(
				(ccode >= 'a'.charCodeAt(0) && ccode <= 'z'.charCodeAt(0))
				|| (ccode >= 'A'.charCodeAt(0) && ccode <= 'Z'.charCodeAt(0))
				|| (ccode == '_'.charCodeAt(0))
			){
				ReadSym();
				break;
			}
			//fprintf(stderr, "Unexpected token '%c'\n", *cur);
			//return(false);
		}
		++cur;
	}

	return(streamOfTokens);
};
module.exports.tokenize = tokenize;
