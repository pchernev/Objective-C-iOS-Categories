
/************** Begin file parse.c *******************************************/
/* Driver template for the LEMON parser generator.
 ** The author disclaims copyright to this source code.
 **
 ** This version of "lempar.c" is modified, slightly, for use by SQLite.
 ** The only modifications are the addition of a couple of NEVER()
 ** macros to disable tests that are needed in the case of a general
 ** LALR(1) grammar but which are always false in the
 ** specific grammar used by SQLite.
 */
/* First off, code is included that follows the "include" declaration
 ** in the input grammar file. */
/* #include <stdio.h> */


/*
 ** Disable all error recovery processing in the parser push-down
 ** automaton.
 */
#define YYNOERRORRECOVERY 1

/*
 ** Make yytestcase() the same as testcase()
 */
#define yytestcase(X) testcase(X)

/*
 ** An instance of this structure holds information about the
 ** LIMIT clause of a SELECT statement.
 */
struct LimitVal {
    Expr *pLimit;    /* The LIMIT expression.  NULL if there is no limit */
    Expr *pOffset;   /* The OFFSET expression.  NULL if there is none */
};

/*
 ** An instance of this structure is used to store the LIKE,
 ** GLOB, NOT LIKE, and NOT GLOB operators.
 */
struct LikeOp {
    Token eOperator;  /* "like" or "glob" or "regexp" */
    int bNot;         /* True if the NOT keyword is present */
};

/*
 ** An instance of the following structure describes the event of a
 ** TRIGGER.  "a" is the event type, one of TK_UPDATE, TK_INSERT,
 ** TK_DELETE, or TK_INSTEAD.  If the event is of the form
 **
 **      UPDATE ON (a,b,c)
 **
 ** Then the "b" IdList records the list "a,b,c".
 */
struct TrigEvent { int a; IdList * b; };

/*
 ** An instance of this structure holds the ATTACH key and the key type.
 */
struct AttachKey { int type;  Token key; };

/*
 ** One or more VALUES claues
 */
struct ValueList {
    ExprList *pList;
    Select *pSelect;
};


/* This is a utility routine used to set the ExprSpan.zStart and
 ** ExprSpan.zEnd values of pOut so that the span covers the complete
 ** range of text beginning with pStart and going to the end of pEnd.
 */
static void spanSet(ExprSpan *pOut, Token *pStart, Token *pEnd){
    pOut->zStart = pStart->z;
    pOut->zEnd = &pEnd->z[pEnd->n];
}

/* Construct a new Expr object from a single identifier.  Use the
 ** new Expr to populate pOut.  Set the span of pOut to be the identifier
 ** that created the expression.
 */
static void spanExpr(ExprSpan *pOut, Parse *pParse, int op, Token *pValue){
    pOut->pExpr = sqlite3PExpr(pParse, op, 0, 0, pValue);
    pOut->zStart = pValue->z;
    pOut->zEnd = &pValue->z[pValue->n];
}

/* This routine constructs a binary expression node out of two ExprSpan
 ** objects and uses the result to populate a new ExprSpan object.
 */
static void spanBinaryExpr(
                           ExprSpan *pOut,     /* Write the result here */
                           Parse *pParse,      /* The parsing context.  Errors accumulate here */
                           int op,             /* The binary operation */
                           ExprSpan *pLeft,    /* The left operand */
                           ExprSpan *pRight    /* The right operand */
){
    pOut->pExpr = sqlite3PExpr(pParse, op, pLeft->pExpr, pRight->pExpr, 0);
    pOut->zStart = pLeft->zStart;
    pOut->zEnd = pRight->zEnd;
}

/* Construct an expression node for a unary postfix operator
 */
static void spanUnaryPostfix(
                             ExprSpan *pOut,        /* Write the new expression node here */
                             Parse *pParse,         /* Parsing context to record errors */
                             int op,                /* The operator */
                             ExprSpan *pOperand,    /* The operand */
                             Token *pPostOp         /* The operand token for setting the span */
){
    pOut->pExpr = sqlite3PExpr(pParse, op, pOperand->pExpr, 0, 0);
    pOut->zStart = pOperand->zStart;
    pOut->zEnd = &pPostOp->z[pPostOp->n];
}

/* A routine to convert a binary TK_IS or TK_ISNOT expression into a
 ** unary TK_ISNULL or TK_NOTNULL expression. */
static void binaryToUnaryIfNull(Parse *pParse, Expr *pY, Expr *pA, int op){
    sqlite3 *db = pParse->db;
    if( db->mallocFailed==0 && pY->op==TK_NULL ){
        pA->op = (u8)op;
        sqlite3ExprDelete(db, pA->pRight);
        pA->pRight = 0;
    }
}

/* Construct an expression node for a unary prefix operator
 */
static void spanUnaryPrefix(
                            ExprSpan *pOut,        /* Write the new expression node here */
                            Parse *pParse,         /* Parsing context to record errors */
                            int op,                /* The operator */
                            ExprSpan *pOperand,    /* The operand */
                            Token *pPreOp         /* The operand token for setting the span */
){
    pOut->pExpr = sqlite3PExpr(pParse, op, pOperand->pExpr, 0, 0);
    pOut->zStart = pPreOp->z;
    pOut->zEnd = pOperand->zEnd;
}
/* Next is all token values, in a form suitable for use by makeheaders.
 ** This section will be null unless lemon is run with the -m switch.
 */
/*
 ** These constants (all generated automatically by the parser generator)
 ** specify the various kinds of tokens (terminals) that the parser
 ** understands.
 **
 ** Each symbol here is a terminal symbol in the grammar.
 */
/* Make sure the INTERFACE macro is defined.
 */
#ifndef INTERFACE
# define INTERFACE 1
#endif
/* The next thing included is series of defines which control
 ** various aspects of the generated parser.
 **    YYCODETYPE         is the data type used for storing terminal
 **                       and nonterminal numbers.  "unsigned char" is
 **                       used if there are fewer than 250 terminals
 **                       and nonterminals.  "int" is used otherwise.
 **    YYNOCODE           is a number of type YYCODETYPE which corresponds
 **                       to no legal terminal or nonterminal number.  This
 **                       number is used to fill in empty slots of the hash
 **                       table.
 **    YYFALLBACK         If defined, this indicates that one or more tokens
 **                       have fall-back values which should be used if the
 **                       original value of the token will not parse.
 **    YYACTIONTYPE       is the data type used for storing terminal
 **                       and nonterminal numbers.  "unsigned char" is
 **                       used if there are fewer than 250 rules and
 **                       states combined.  "int" is used otherwise.
 **    sqlite3ParserTOKENTYPE     is the data type used for minor tokens given
 **                       directly to the parser from the tokenizer.
 **    YYMINORTYPE        is the data type used for all minor tokens.
 **                       This is typically a union of many types, one of
 **                       which is sqlite3ParserTOKENTYPE.  The entry in the union
 **                       for base tokens is called "yy0".
 **    YYSTACKDEPTH       is the maximum depth of the parser's stack.  If
 **                       zero the stack is dynamically sized using realloc()
 **    sqlite3ParserARG_SDECL     A static variable declaration for the %extra_argument
 **    sqlite3ParserARG_PDECL     A parameter declaration for the %extra_argument
 **    sqlite3ParserARG_STORE     Code to store %extra_argument into yypParser
 **    sqlite3ParserARG_FETCH     Code to extract %extra_argument from yypParser
 **    YYNSTATE           the combined number of states.
 **    YYNRULE            the number of rules in the grammar
 **    YYERRORSYMBOL      is the code number of the error symbol.  If not
 **                       defined, then do no error processing.
 */
#define YYCODETYPE unsigned char
#define YYNOCODE 251
#define YYACTIONTYPE unsigned short int
#define YYWILDCARD 67
#define sqlite3ParserTOKENTYPE Token
typedef union {
    int yyinit;
    sqlite3ParserTOKENTYPE yy0;
    struct LimitVal yy64;
    Expr* yy122;
    Select* yy159;
    IdList* yy180;
    struct {int value; int mask;} yy207;
    u8 yy258;
    u16 yy305;
    struct LikeOp yy318;
    TriggerStep* yy327;
    ExprSpan yy342;
    SrcList* yy347;
    int yy392;
    struct TrigEvent yy410;
    ExprList* yy442;
    struct ValueList yy487;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define sqlite3ParserARG_SDECL Parse *pParse;
#define sqlite3ParserARG_PDECL ,Parse *pParse
#define sqlite3ParserARG_FETCH Parse *pParse = yypParser->pParse
#define sqlite3ParserARG_STORE yypParser->pParse = pParse
#define YYNSTATE 628
#define YYNRULE 327
#define YYFALLBACK 1
#define YY_NO_ACTION      (YYNSTATE+YYNRULE+2)
#define YY_ACCEPT_ACTION  (YYNSTATE+YYNRULE+1)
#define YY_ERROR_ACTION   (YYNSTATE+YYNRULE)

/* The yyzerominor constant is used to initialize instances of
 ** YYMINORTYPE objects to zero. */
static const YYMINORTYPE yyzerominor = { 0 };

/* Define the yytestcase() macro to be a no-op if is not already defined
 ** otherwise.
 **
 ** Applications can choose to define yytestcase() in the %include section
 ** to a macro that can assist in verifying code coverage.  For production
 ** code the yytestcase() macro should be turned off.  But it is useful
 ** for testing.
 */
#ifndef yytestcase
# define yytestcase(X)
#endif


/* Next are the tables used to determine what action to take based on the
 ** current state and lookahead token.  These tables are used to implement
 ** functions that take a state number and lookahead value and return an
 ** action integer.
 **
 ** Suppose the action integer is N.  Then the action is determined as
 ** follows
 **
 **   0 <= N < YYNSTATE                  Shift N.  That is, push the lookahead
 **                                      token onto the stack and goto state N.
 **
 **   YYNSTATE <= N < YYNSTATE+YYNRULE   Reduce by rule N-YYNSTATE.
 **
 **   N == YYNSTATE+YYNRULE              A syntax error has occurred.
 **
 **   N == YYNSTATE+YYNRULE+1            The parser accepts its input.
 **
 **   N == YYNSTATE+YYNRULE+2            No such action.  Denotes unused
 **                                      slots in the yy_action[] table.
 **
 ** The action table is constructed as a single large table named yy_action[].
 ** Given state S and lookahead X, the action is computed as
 **
 **      yy_action[ yy_shift_ofst[S] + X ]
 **
 ** If the index value yy_shift_ofst[S]+X is out of range or if the value
 ** yy_lookahead[yy_shift_ofst[S]+X] is not equal to X or if yy_shift_ofst[S]
 ** is equal to YY_SHIFT_USE_DFLT, it means that the action is not in the table
 ** and that yy_default[S] should be used instead.
 **
 ** The formula above is for computing the action when the lookahead is
 ** a terminal symbol.  If the lookahead is a non-terminal (as occurs after
 ** a reduce action) then the yy_reduce_ofst[] array is used in place of
 ** the yy_shift_ofst[] array and YY_REDUCE_USE_DFLT is used in place of
 ** YY_SHIFT_USE_DFLT.
 **
 ** The following are the tables generated in this section:
 **
 **  yy_action[]        A single table containing all actions.
 **  yy_lookahead[]     A table containing the lookahead for each entry in
 **                     yy_action.  Used to detect hash collisions.
 **  yy_shift_ofst[]    For each state, the offset into yy_action for
 **                     shifting terminals.
 **  yy_reduce_ofst[]   For each state, the offset into yy_action for
 **                     shifting non-terminals after a reduce.
 **  yy_default[]       Default action for each state.
 */
#define YY_ACTTAB_COUNT (1564)
static const YYACTIONTYPE yy_action[] = {
    /*     0 */   310,  956,  184,  418,    2,  171,  625,  595,   56,   56,
    /*    10 */    56,   56,   49,   54,   54,   54,   54,   53,   53,   52,
    /*    20 */    52,   52,   51,  233,  621,  620,  299,  621,  620,  234,
    /*    30 */   588,  582,   56,   56,   56,   56,   19,   54,   54,   54,
    /*    40 */    54,   53,   53,   52,   52,   52,   51,  233,  606,   57,
    /*    50 */    58,   48,  580,  579,  581,  581,   55,   55,   56,   56,
    /*    60 */    56,   56,  542,   54,   54,   54,   54,   53,   53,   52,
    /*    70 */    52,   52,   51,  233,  310,  595,  326,  196,  195,  194,
    /*    80 */    33,   54,   54,   54,   54,   53,   53,   52,   52,   52,
    /*    90 */    51,  233,  618,  617,  165,  618,  617,  381,  378,  377,
    /*   100 */   408,  533,  577,  577,  588,  582,  304,  423,  376,   59,
    /*   110 */    53,   53,   52,   52,   52,   51,  233,   50,   47,  146,
    /*   120 */   575,  546,   65,   57,   58,   48,  580,  579,  581,  581,
    /*   130 */    55,   55,   56,   56,   56,   56,  213,   54,   54,   54,
    /*   140 */    54,   53,   53,   52,   52,   52,   51,  233,  310,  223,
    /*   150 */   540,  421,  170,  176,  138,  281,  384,  276,  383,  168,
    /*   160 */   490,  552,  410,  669,  621,  620,  272,  439,  410,  439,
    /*   170 */   551,  605,   67,  483,  508,  619,  600,  413,  588,  582,
    /*   180 */   601,  484,  619,  413,  619,  599,   91,  440,  441,  440,
    /*   190 */   336,  599,   73,  670,  222,  267,  481,   57,   58,   48,
    /*   200 */   580,  579,  581,  581,   55,   55,   56,   56,   56,   56,
    /*   210 */   671,   54,   54,   54,   54,   53,   53,   52,   52,   52,
    /*   220 */    51,  233,  310,  280,  232,  231,    1,  132,  200,  386,
    /*   230 */   621,  620,  618,  617,  279,  436,  290,  564,  175,  263,
    /*   240 */   410,  265,  438,  498,  437,  166,  442,  569,  337,  569,
    /*   250 */   201,  538,  588,  582,  600,  413,  165,  595,  601,  381,
    /*   260 */   378,  377,  598,  599,   92,  524,  619,  570,  570,  593,
    /*   270 */   376,   57,   58,   48,  580,  579,  581,  581,   55,   55,
    /*   280 */    56,   56,   56,   56,  598,   54,   54,   54,   54,   53,
    /*   290 */    53,   52,   52,   52,   51,  233,  310,  464,  618,  617,
    /*   300 */   591,  591,  591,  174,  273,  397,  410,  273,  410,  549,
    /*   310 */   398,  621,  620,   68,  327,  621,  620,  621,  620,  619,
    /*   320 */   547,  413,  619,  413,  472,  595,  588,  582,  473,  599,
    /*   330 */    92,  599,   92,   52,   52,   52,   51,  233,  514,  513,
    /*   340 */   206,  323,  364,  465,  221,   57,   58,   48,  580,  579,
    /*   350 */   581,  581,   55,   55,   56,   56,   56,   56,  530,   54,
    /*   360 */    54,   54,   54,   53,   53,   52,   52,   52,   51,  233,
    /*   370 */   310,  397,  410,  397,  598,  373,  387,  531,  348,  618,
    /*   380 */   617,  576,  202,  618,  617,  618,  617,  413,  621,  620,
    /*   390 */   145,  255,  347,  254,  578,  599,   74,  352,   45,  490,
    /*   400 */   588,  582,  235,  189,  465,  545,  167,  297,  187,  470,
    /*   410 */   480,   67,   62,   39,  619,  547,  598,  346,  574,   57,
    /*   420 */    58,   48,  580,  579,  581,  581,   55,   55,   56,   56,
    /*   430 */    56,   56,    6,   54,   54,   54,   54,   53,   53,   52,
    /*   440 */    52,   52,   51,  233,  310,  563,  559,  408,  529,  577,
    /*   450 */   577,  345,  255,  347,  254,  182,  618,  617,  504,  505,
    /*   460 */   315,  410,  558,  235,  166,  272,  410,  353,  565,  181,
    /*   470 */   408,  547,  577,  577,  588,  582,  413,  538,  557,  562,
    /*   480 */   518,  413,  619,  249,  599,   16,    7,   36,  468,  599,
    /*   490 */    92,  517,  619,   57,   58,   48,  580,  579,  581,  581,
    /*   500 */    55,   55,   56,   56,   56,   56,  542,   54,   54,   54,
    /*   510 */    54,   53,   53,   52,   52,   52,   51,  233,  310,  328,
    /*   520 */   573,  572,  526,  559,  561,  395,  872,  246,  410,  248,
    /*   530 */   171,  393,  595,  219,  408,  410,  577,  577,  503,  558,
    /*   540 */   365,  145,  511,  413,  408,  229,  577,  577,  588,  582,
    /*   550 */   413,  599,   92,  382,  270,  557,  166,  401,  599,   69,
    /*   560 */   502,  420,  946,  199,  946,  198,  547,   57,   58,   48,
    /*   570 */   580,  579,  581,  581,   55,   55,   56,   56,   56,   56,
    /*   580 */   569,   54,   54,   54,   54,   53,   53,   52,   52,   52,
    /*   590 */    51,  233,  310,  318,  420,  945,  509,  945,  309,  598,
    /*   600 */   595,  566,  491,  212,  173,  247,  424,  616,  615,  614,
    /*   610 */   324,  197,  143,  406,  573,  572,  490,   66,   50,   47,
    /*   620 */   146,  595,  588,  582,  232,  231,  560,  428,   67,  556,
    /*   630 */    15,  619,  186,  544,  304,  422,   35,  206,  433,  424,
    /*   640 */   553,   57,   58,   48,  580,  579,  581,  581,   55,   55,
    /*   650 */    56,   56,   56,   56,  205,   54,   54,   54,   54,   53,
    /*   660 */    53,   52,   52,   52,   51,  233,  310,  570,  570,  261,
    /*   670 */   269,  598,   12,  374,  569,  166,  410,  314,  410,  421,
    /*   680 */   410,  474,  474,  366,  619,   50,   47,  146,  598,  595,
    /*   690 */   256,  413,  166,  413,  352,  413,  588,  582,   32,  599,
    /*   700 */    94,  599,   97,  599,   95,  628,  626,  330,  142,   50,
    /*   710 */    47,  146,  334,  350,  359,   57,   58,   48,  580,  579,
    /*   720 */   581,  581,   55,   55,   56,   56,   56,   56,  410,   54,
    /*   730 */    54,   54,   54,   53,   53,   52,   52,   52,   51,  233,
    /*   740 */   310,  410,  389,  413,  410,   22,  566,  405,  212,  363,
    /*   750 */   390,  599,  104,  360,  410,  156,  413,  410,  604,  413,
    /*   760 */   538,  332,  570,  570,  599,  103,  494,  599,  105,  413,
    /*   770 */   588,  582,  413,  261,  550,  619,   11,  599,  106,  522,
    /*   780 */   599,  133,  169,  458,  457,  170,   35,  602,  619,   57,
    /*   790 */    58,   48,  580,  579,  581,  581,   55,   55,   56,   56,
    /*   800 */    56,   56,  410,   54,   54,   54,   54,   53,   53,   52,
    /*   810 */    52,   52,   51,  233,  310,  410,  260,  413,  410,   50,
    /*   820 */    47,  146,  358,  319,  356,  599,  134,  528,  353,  338,
    /*   830 */   413,  410,  357,  413,  358,  410,  358,  619,  599,   98,
    /*   840 */   129,  599,  102,  619,  588,  582,  413,   21,  235,  619,
    /*   850 */   413,  619,  211,  143,  599,  101,   30,  167,  599,   93,
    /*   860 */   351,  536,  203,   57,   58,   48,  580,  579,  581,  581,
    /*   870 */    55,   55,   56,   56,   56,   56,  410,   54,   54,   54,
    /*   880 */    54,   53,   53,   52,   52,   52,   51,  233,  310,  410,
    /*   890 */   527,  413,  410,  426,  215,  306,  598,  552,  141,  599,
    /*   900 */   100,   40,  410,   38,  413,  410,  551,  413,  410,  228,
    /*   910 */   220,  315,  599,   77,  501,  599,   96,  413,  588,  582,
    /*   920 */   413,  339,  253,  413,  218,  599,  137,  380,  599,  136,
    /*   930 */    28,  599,  135,  271,  716,  210,  482,   57,   58,   48,
    /*   940 */   580,  579,  581,  581,   55,   55,   56,   56,   56,   56,
    /*   950 */   410,   54,   54,   54,   54,   53,   53,   52,   52,   52,
    /*   960 */    51,  233,  310,  410,  273,  413,  410,  316,  147,  598,
    /*   970 */   273,  627,    2,  599,   76,  209,  410,  127,  413,  619,
    /*   980 */   126,  413,  410,  622,  235,  619,  599,   90,  375,  599,
    /*   990 */    89,  413,  588,  582,   27,  261,  351,  413,  619,  599,
    /*  1000 */    75,  322,  542,  542,  125,  599,   88,  321,  279,  598,
    /*  1010 */   619,   57,   46,   48,  580,  579,  581,  581,   55,   55,
    /*  1020 */    56,   56,   56,   56,  410,   54,   54,   54,   54,   53,
    /*  1030 */    53,   52,   52,   52,   51,  233,  310,  410,  451,  413,
    /*  1040 */   164,  285,  283,  273,  610,  425,  305,  599,   87,  371,
    /*  1050 */   410,  478,  413,  410,  609,  410,  608,  603,  619,  619,
    /*  1060 */   599,   99,  587,  586,  122,  413,  588,  582,  413,  619,
    /*  1070 */   413,  619,  619,  599,   86,  367,  599,   17,  599,   85,
    /*  1080 */   320,  185,  520,  519,  584,  583,   58,   48,  580,  579,
    /*  1090 */   581,  581,   55,   55,   56,   56,   56,   56,  410,   54,
    /*  1100 */    54,   54,   54,   53,   53,   52,   52,   52,   51,  233,
    /*  1110 */   310,  585,  410,  413,  410,  261,  261,  261,  409,  592,
    /*  1120 */   475,  599,   84,  170,  410,  467,  519,  413,  121,  413,
    /*  1130 */   619,  619,  619,  619,  619,  599,   83,  599,   72,  413,
    /*  1140 */   588,  582,   51,  233,  626,  330,  471,  599,   71,  258,
    /*  1150 */   159,  120,   14,  463,  157,  158,  117,  261,  449,  448,
    /*  1160 */   447,   48,  580,  579,  581,  581,   55,   55,   56,   56,
    /*  1170 */    56,   56,  619,   54,   54,   54,   54,   53,   53,   52,
    /*  1180 */    52,   52,   51,  233,   44,  404,  261,    3,  410,  460,
    /*  1190 */   261,  414,  620,  118,  399,   10,   25,   24,  555,  349,
    /*  1200 */   217,  619,  407,  413,  410,  619,    4,   44,  404,  619,
    /*  1210 */     3,  599,   82,  619,  414,  620,  456,  543,  115,  413,
    /*  1220 */   539,  402,  537,  275,  507,  407,  251,  599,   81,  216,
    /*  1230 */   274,  564,  619,  243,  454,  619,  154,  619,  619,  619,
    /*  1240 */   450,  417,  624,  110,  402,  619,  410,  236,   64,  123,
    /*  1250 */   488,   41,   42,  532,  564,  204,  410,  268,   43,  412,
    /*  1260 */   411,  413,  266,  593,  108,  619,  107,  435,  333,  599,
    /*  1270 */    80,  413,  619,  264,   41,   42,  444,  619,  410,  599,
    /*  1280 */    70,   43,  412,  411,  434,  262,  593,  149,  619,  598,
    /*  1290 */   257,  237,  188,  413,  591,  591,  591,  590,  589,   13,
    /*  1300 */   619,  599,   18,  329,  235,  619,   44,  404,  361,    3,
    /*  1310 */   419,  462,  340,  414,  620,  227,  124,  591,  591,  591,
    /*  1320 */   590,  589,   13,  619,  407,  410,  619,  410,  139,   34,
    /*  1330 */   404,  388,    3,  148,  623,  313,  414,  620,  312,  331,
    /*  1340 */   413,  461,  413,  402,  180,  354,  413,  407,  599,   79,
    /*  1350 */   599,   78,  250,  564,  599,    9,  619,  613,  612,  611,
    /*  1360 */   619,    8,  453,  443,  242,  416,  402,  619,  239,  235,
    /*  1370 */   179,  238,  429,   41,   42,  289,  564,  619,  619,  619,
    /*  1380 */    43,  412,  411,  619,  144,  593,  619,  619,  177,   61,
    /*  1390 */   619,  597,  392,  621,  620,  288,   41,   42,  415,  619,
    /*  1400 */   294,   30,  394,   43,  412,  411,  293,  619,  593,   31,
    /*  1410 */   619,  396,  292,   60,  230,   37,  591,  591,  591,  590,
    /*  1420 */   589,   13,  214,  554,  183,  291,  172,  302,  301,  300,
    /*  1430 */   178,  298,  596,  564,  452,   29,  286,  391,  541,  591,
    /*  1440 */   591,  591,  590,  589,   13,  284,  521,  535,  150,  534,
    /*  1450 */   241,  282,  385,  192,  191,  325,  516,  515,  277,  240,
    /*  1460 */   511,  524,  308,  512,  128,  593,  510,  225,  226,  487,
    /*  1470 */   486,  224,  152,  492,  465,  307,  485,  163,  153,  372,
    /*  1480 */   479,  151,  162,  259,  370,  161,  368,  208,  476,  477,
    /*  1490 */    26,  160,  469,  466,  362,  140,  591,  591,  591,  116,
    /*  1500 */   119,  455,  344,  155,  114,  343,  113,  112,  446,  111,
    /*  1510 */   131,  109,  432,  317,  130,  431,   23,   20,  430,  427,
    /*  1520 */   190,   63,  255,  342,  244,  607,  295,  287,  311,  594,
    /*  1530 */   278,  508,  496,  235,  493,  571,  497,  568,  495,  403,
    /*  1540 */   459,  379,  355,  245,  193,  303,  567,  296,  341,    5,
    /*  1550 */   445,  548,  506,  207,  525,  500,  335,  489,  252,  369,
    /*  1560 */   400,  499,  523,  233,
};
static const YYCODETYPE yy_lookahead[] = {
    /*     0 */    19,  142,  143,  144,  145,   24,    1,   26,   77,   78,
    /*    10 */    79,   80,   81,   82,   83,   84,   85,   86,   87,   88,
    /*    20 */    89,   90,   91,   92,   26,   27,   15,   26,   27,  197,
    /*    30 */    49,   50,   77,   78,   79,   80,  204,   82,   83,   84,
    /*    40 */    85,   86,   87,   88,   89,   90,   91,   92,   23,   68,
    /*    50 */    69,   70,   71,   72,   73,   74,   75,   76,   77,   78,
    /*    60 */    79,   80,  166,   82,   83,   84,   85,   86,   87,   88,
    /*    70 */    89,   90,   91,   92,   19,   94,   19,  105,  106,  107,
    /*    80 */    25,   82,   83,   84,   85,   86,   87,   88,   89,   90,
    /*    90 */    91,   92,   94,   95,   96,   94,   95,   99,  100,  101,
    /*   100 */   112,  205,  114,  115,   49,   50,   22,   23,  110,   54,
    /*   110 */    86,   87,   88,   89,   90,   91,   92,  221,  222,  223,
    /*   120 */    23,  120,   25,   68,   69,   70,   71,   72,   73,   74,
    /*   130 */    75,   76,   77,   78,   79,   80,   22,   82,   83,   84,
    /*   140 */    85,   86,   87,   88,   89,   90,   91,   92,   19,   92,
    /*   150 */    23,   67,   25,   96,   97,   98,   99,  100,  101,  102,
    /*   160 */   150,   32,  150,  118,   26,   27,  109,  150,  150,  150,
    /*   170 */    41,  161,  162,  180,  181,  165,  113,  165,   49,   50,
    /*   180 */   117,  188,  165,  165,  165,  173,  174,  170,  171,  170,
    /*   190 */   171,  173,  174,  118,  184,   16,  186,   68,   69,   70,
    /*   200 */    71,   72,   73,   74,   75,   76,   77,   78,   79,   80,
    /*   210 */   118,   82,   83,   84,   85,   86,   87,   88,   89,   90,
    /*   220 */    91,   92,   19,   98,   86,   87,   22,   24,  160,   88,
    /*   230 */    26,   27,   94,   95,  109,   97,  224,   66,  118,   60,
    /*   240 */   150,   62,  104,   23,  106,   25,  229,  230,  229,  230,
    /*   250 */   160,  150,   49,   50,  113,  165,   96,   26,  117,   99,
    /*   260 */   100,  101,  194,  173,  174,   94,  165,  129,  130,   98,
    /*   270 */   110,   68,   69,   70,   71,   72,   73,   74,   75,   76,
    /*   280 */    77,   78,   79,   80,  194,   82,   83,   84,   85,   86,
    /*   290 */    87,   88,   89,   90,   91,   92,   19,   11,   94,   95,
    /*   300 */   129,  130,  131,  118,  150,  215,  150,  150,  150,   25,
    /*   310 */   220,   26,   27,   22,  213,   26,   27,   26,   27,  165,
    /*   320 */    25,  165,  165,  165,   30,   94,   49,   50,   34,  173,
    /*   330 */   174,  173,  174,   88,   89,   90,   91,   92,    7,    8,
    /*   340 */   160,  187,   48,   57,  187,   68,   69,   70,   71,   72,
    /*   350 */    73,   74,   75,   76,   77,   78,   79,   80,   23,   82,
    /*   360 */    83,   84,   85,   86,   87,   88,   89,   90,   91,   92,
    /*   370 */    19,  215,  150,  215,  194,   19,  220,   88,  220,   94,
    /*   380 */    95,   23,  160,   94,   95,   94,   95,  165,   26,   27,
    /*   390 */    95,  105,  106,  107,  113,  173,  174,  217,   22,  150,
    /*   400 */    49,   50,  116,  119,   57,  120,   50,  158,   22,   21,
    /*   410 */   161,  162,  232,  136,  165,  120,  194,  237,   23,   68,
    /*   420 */    69,   70,   71,   72,   73,   74,   75,   76,   77,   78,
    /*   430 */    79,   80,   22,   82,   83,   84,   85,   86,   87,   88,
    /*   440 */    89,   90,   91,   92,   19,   23,   12,  112,   23,  114,
    /*   450 */   115,   63,  105,  106,  107,   23,   94,   95,   97,   98,
    /*   460 */   104,  150,   28,  116,   25,  109,  150,  150,   23,   23,
    /*   470 */   112,   25,  114,  115,   49,   50,  165,  150,   44,   11,
    /*   480 */    46,  165,  165,   16,  173,  174,   76,  136,  100,  173,
    /*   490 */   174,   57,  165,   68,   69,   70,   71,   72,   73,   74,
    /*   500 */    75,   76,   77,   78,   79,   80,  166,   82,   83,   84,
    /*   510 */    85,   86,   87,   88,   89,   90,   91,   92,   19,  169,
    /*   520 */   170,  171,   23,   12,   23,  214,  138,   60,  150,   62,
    /*   530 */    24,  215,   26,  216,  112,  150,  114,  115,   36,   28,
    /*   540 */   213,   95,  103,  165,  112,  205,  114,  115,   49,   50,
    /*   550 */   165,  173,  174,   51,   23,   44,   25,   46,  173,  174,
    /*   560 */    58,   22,   23,   22,   25,  160,  120,   68,   69,   70,
    /*   570 */    71,   72,   73,   74,   75,   76,   77,   78,   79,   80,
    /*   580 */   230,   82,   83,   84,   85,   86,   87,   88,   89,   90,
    /*   590 */    91,   92,   19,  215,   22,   23,   23,   25,  163,  194,
    /*   600 */    94,  166,  167,  168,   25,  138,   67,    7,    8,    9,
    /*   610 */   108,  206,  207,  169,  170,  171,  150,   22,  221,  222,
    /*   620 */   223,   26,   49,   50,   86,   87,   23,  161,  162,   23,
    /*   630 */    22,  165,   24,  120,   22,   23,   25,  160,  241,   67,
    /*   640 */   176,   68,   69,   70,   71,   72,   73,   74,   75,   76,
    /*   650 */    77,   78,   79,   80,  160,   82,   83,   84,   85,   86,
    /*   660 */    87,   88,   89,   90,   91,   92,   19,  129,  130,  150,
    /*   670 */    23,  194,   35,   23,  230,   25,  150,  155,  150,   67,
    /*   680 */   150,  105,  106,  107,  165,  221,  222,  223,  194,   94,
    /*   690 */    23,  165,   25,  165,  217,  165,   49,   50,   25,  173,
    /*   700 */   174,  173,  174,  173,  174,    0,    1,    2,  118,  221,
    /*   710 */   222,  223,  193,  219,  237,   68,   69,   70,   71,   72,
    /*   720 */    73,   74,   75,   76,   77,   78,   79,   80,  150,   82,
    /*   730 */    83,   84,   85,   86,   87,   88,   89,   90,   91,   92,
    /*   740 */    19,  150,   19,  165,  150,   24,  166,  167,  168,  227,
    /*   750 */    27,  173,  174,  231,  150,   25,  165,  150,  172,  165,
    /*   760 */   150,  242,  129,  130,  173,  174,  180,  173,  174,  165,
    /*   770 */    49,   50,  165,  150,  176,  165,   35,  173,  174,  165,
    /*   780 */   173,  174,   35,   23,   23,   25,   25,  173,  165,   68,
    /*   790 */    69,   70,   71,   72,   73,   74,   75,   76,   77,   78,
    /*   800 */    79,   80,  150,   82,   83,   84,   85,   86,   87,   88,
    /*   810 */    89,   90,   91,   92,   19,  150,  193,  165,  150,  221,
    /*   820 */   222,  223,  150,  213,   19,  173,  174,   23,  150,   97,
    /*   830 */   165,  150,   27,  165,  150,  150,  150,  165,  173,  174,
    /*   840 */    22,  173,  174,  165,   49,   50,  165,   52,  116,  165,
    /*   850 */   165,  165,  206,  207,  173,  174,  126,   50,  173,  174,
    /*   860 */   128,   27,  160,   68,   69,   70,   71,   72,   73,   74,
    /*   870 */    75,   76,   77,   78,   79,   80,  150,   82,   83,   84,
    /*   880 */    85,   86,   87,   88,   89,   90,   91,   92,   19,  150,
    /*   890 */    23,  165,  150,   23,  216,   25,  194,   32,   39,  173,
    /*   900 */   174,  135,  150,  137,  165,  150,   41,  165,  150,   52,
    /*   910 */   238,  104,  173,  174,   29,  173,  174,  165,   49,   50,
    /*   920 */   165,  219,  238,  165,  238,  173,  174,   52,  173,  174,
    /*   930 */    22,  173,  174,   23,   23,  160,   25,   68,   69,   70,
    /*   940 */    71,   72,   73,   74,   75,   76,   77,   78,   79,   80,
    /*   950 */   150,   82,   83,   84,   85,   86,   87,   88,   89,   90,
    /*   960 */    91,   92,   19,  150,  150,  165,  150,  245,  246,  194,
    /*   970 */   150,  144,  145,  173,  174,  160,  150,   22,  165,  165,
    /*   980 */    22,  165,  150,  150,  116,  165,  173,  174,   52,  173,
    /*   990 */   174,  165,   49,   50,   22,  150,  128,  165,  165,  173,
    /*  1000 */   174,  187,  166,  166,   22,  173,  174,  187,  109,  194,
    /*  1010 */   165,   68,   69,   70,   71,   72,   73,   74,   75,   76,
    /*  1020 */    77,   78,   79,   80,  150,   82,   83,   84,   85,   86,
    /*  1030 */    87,   88,   89,   90,   91,   92,   19,  150,  193,  165,
    /*  1040 */   102,  205,  205,  150,  150,  247,  248,  173,  174,   19,
    /*  1050 */   150,   20,  165,  150,  150,  150,  150,  150,  165,  165,
    /*  1060 */   173,  174,   49,   50,  104,  165,   49,   50,  165,  165,
    /*  1070 */   165,  165,  165,  173,  174,   43,  173,  174,  173,  174,
    /*  1080 */   187,   24,  190,  191,   71,   72,   69,   70,   71,   72,
    /*  1090 */    73,   74,   75,   76,   77,   78,   79,   80,  150,   82,
    /*  1100 */    83,   84,   85,   86,   87,   88,   89,   90,   91,   92,
    /*  1110 */    19,   98,  150,  165,  150,  150,  150,  150,  150,  150,
    /*  1120 */    59,  173,  174,   25,  150,  190,  191,  165,   53,  165,
    /*  1130 */   165,  165,  165,  165,  165,  173,  174,  173,  174,  165,
    /*  1140 */    49,   50,   91,   92,    1,    2,   53,  173,  174,  138,
    /*  1150 */   104,   22,    5,    1,   35,  118,  127,  150,  193,  193,
    /*  1160 */   193,   70,   71,   72,   73,   74,   75,   76,   77,   78,
    /*  1170 */    79,   80,  165,   82,   83,   84,   85,   86,   87,   88,
    /*  1180 */    89,   90,   91,   92,   19,   20,  150,   22,  150,   27,
    /*  1190 */   150,   26,   27,  108,  150,   22,   76,   76,  150,   25,
    /*  1200 */   193,  165,   37,  165,  150,  165,   22,   19,   20,  165,
    /*  1210 */    22,  173,  174,  165,   26,   27,   23,  150,  119,  165,
    /*  1220 */   150,   56,  150,  150,  150,   37,   16,  173,  174,  193,
    /*  1230 */   150,   66,  165,  193,    1,  165,  121,  165,  165,  165,
    /*  1240 */    20,  146,  147,  119,   56,  165,  150,  152,   16,  154,
    /*  1250 */   150,   86,   87,   88,   66,  160,  150,  150,   93,   94,
    /*  1260 */    95,  165,  150,   98,  108,  165,  127,   23,   65,  173,
    /*  1270 */   174,  165,  165,  150,   86,   87,  128,  165,  150,  173,
    /*  1280 */   174,   93,   94,   95,   23,  150,   98,   15,  165,  194,
    /*  1290 */   150,  140,   22,  165,  129,  130,  131,  132,  133,  134,
    /*  1300 */   165,  173,  174,    3,  116,  165,   19,   20,  150,   22,
    /*  1310 */     4,  150,  217,   26,   27,  179,  179,  129,  130,  131,
    /*  1320 */   132,  133,  134,  165,   37,  150,  165,  150,  164,   19,
    /*  1330 */    20,  150,   22,  246,  149,  249,   26,   27,  249,  244,
    /*  1340 */   165,  150,  165,   56,    6,  150,  165,   37,  173,  174,
    /*  1350 */   173,  174,  150,   66,  173,  174,  165,  149,  149,   13,
    /*  1360 */   165,   25,  150,  150,  150,  149,   56,  165,  150,  116,
    /*  1370 */   151,  150,  150,   86,   87,  150,   66,  165,  165,  165,
    /*  1380 */    93,   94,   95,  165,  150,   98,  165,  165,  151,   22,
    /*  1390 */   165,  194,  150,   26,   27,  150,   86,   87,  159,  165,
    /*  1400 */   199,  126,  123,   93,   94,   95,  200,  165,   98,  124,
    /*  1410 */   165,  122,  201,  125,  225,  135,  129,  130,  131,  132,
    /*  1420 */   133,  134,    5,  157,  157,  202,  118,   10,   11,   12,
    /*  1430 */    13,   14,  203,   66,   17,  104,  210,  121,  211,  129,
    /*  1440 */   130,  131,  132,  133,  134,  210,  175,  211,   31,  211,
    /*  1450 */    33,  210,  104,   86,   87,   47,  175,  183,  175,   42,
    /*  1460 */   103,   94,  178,  177,   22,   98,  175,   92,  228,  175,
    /*  1470 */   175,  228,   55,  183,   57,  178,  175,  156,   61,   18,
    /*  1480 */   157,   64,  156,  235,  157,  156,   45,  157,  236,  157,
    /*  1490 */   135,  156,  199,  189,  157,   68,  129,  130,  131,   22,
    /*  1500 */   189,  199,  157,  156,  192,   18,  192,  192,  199,  192,
    /*  1510 */   218,  189,   40,  157,  218,  157,  240,  240,  157,   38,
    /*  1520 */   196,  243,  105,  106,  107,  153,  198,  209,  111,  166,
    /*  1530 */   176,  181,  166,  116,  166,  230,  176,  230,  176,  226,
    /*  1540 */   199,  177,  239,  209,  185,  148,  166,  195,  209,  196,
    /*  1550 */   199,  208,  182,  233,  173,  182,  139,  186,  239,  234,
    /*  1560 */   191,  182,  173,   92,
};
#define YY_SHIFT_USE_DFLT (-70)
#define YY_SHIFT_COUNT (417)
#define YY_SHIFT_MIN   (-69)
#define YY_SHIFT_MAX   (1487)
static const short yy_shift_ofst[] = {
    /*     0 */  1143, 1188, 1417, 1188, 1287, 1287,  138,  138,   -2,  -19,
    /*    10 */  1287, 1287, 1287, 1287,  347,  362,  129,  129,  795, 1165,
    /*    20 */  1287, 1287, 1287, 1287, 1287, 1287, 1287, 1287, 1287, 1287,
    /*    30 */  1287, 1287, 1287, 1287, 1287, 1287, 1287, 1287, 1287, 1287,
    /*    40 */  1287, 1287, 1287, 1287, 1287, 1287, 1287, 1287, 1310, 1287,
    /*    50 */  1287, 1287, 1287, 1287, 1287, 1287, 1287, 1287, 1287, 1287,
    /*    60 */  1287, 1287,  286,  362,  362,  538,  538,  231, 1253,   55,
    /*    70 */   721,  647,  573,  499,  425,  351,  277,  203,  869,  869,
    /*    80 */   869,  869,  869,  869,  869,  869,  869,  869,  869,  869,
    /*    90 */   869,  869,  869,  943,  869, 1017, 1091, 1091,  -69,  -45,
    /*   100 */   -45,  -45,  -45,  -45,   -1,   24,  245,  362,  362,  362,
    /*   110 */   362,  362,  362,  362,  362,  362,  362,  362,  362,  362,
    /*   120 */   362,  362,  362,  388,  356,  362,  362,  362,  362,  362,
    /*   130 */   732,  868,  231, 1051, 1471,  -70,  -70,  -70, 1367,   57,
    /*   140 */   434,  434,  289,  291,  285,    1,  204,  572,  539,  362,
    /*   150 */   362,  362,  362,  362,  362,  362,  362,  362,  362,  362,
    /*   160 */   362,  362,  362,  362,  362,  362,  362,  362,  362,  362,
    /*   170 */   362,  362,  362,  362,  362,  362,  362,  362,  362,  362,
    /*   180 */   362,  506,  506,  506,  705, 1253, 1253, 1253,  -70,  -70,
    /*   190 */   -70,  171,  171,  160,  502,  502,  502,  446,  432,  511,
    /*   200 */   422,  358,  335,  -12,  -12,  -12,  -12,  576,  294,  -12,
    /*   210 */   -12,  295,  595,  141,  600,  730,  723,  723,  805,  730,
    /*   220 */   805,  439,  911,  231,  865,  231,  865,  807,  865,  723,
    /*   230 */   766,  633,  633,  231,  284,   63,  608, 1481, 1308, 1308,
    /*   240 */  1472, 1472, 1308, 1477, 1427, 1275, 1487, 1487, 1487, 1487,
    /*   250 */  1308, 1461, 1275, 1477, 1427, 1427, 1275, 1308, 1461, 1355,
    /*   260 */  1441, 1308, 1308, 1461, 1308, 1461, 1308, 1461, 1442, 1348,
    /*   270 */  1348, 1348, 1408, 1375, 1375, 1442, 1348, 1357, 1348, 1408,
    /*   280 */  1348, 1348, 1316, 1331, 1316, 1331, 1316, 1331, 1308, 1308,
    /*   290 */  1280, 1288, 1289, 1285, 1279, 1275, 1253, 1336, 1346, 1346,
    /*   300 */  1338, 1338, 1338, 1338,  -70,  -70,  -70,  -70,  -70,  -70,
    /*   310 */  1013,  467,  612,   84,  179,  -28,  870,  410,  761,  760,
    /*   320 */   667,  650,  531,  220,  361,  331,  125,  127,   97, 1306,
    /*   330 */  1300, 1270, 1151, 1272, 1203, 1232, 1261, 1244, 1148, 1174,
    /*   340 */  1139, 1156, 1124, 1220, 1115, 1210, 1233, 1099, 1193, 1184,
    /*   350 */  1174, 1173, 1029, 1121, 1120, 1085, 1162, 1119, 1037, 1152,
    /*   360 */  1147, 1129, 1046, 1011, 1093, 1098, 1075, 1061, 1032,  960,
    /*   370 */  1057, 1031, 1030,  899,  938,  982,  936,  972,  958,  910,
    /*   380 */   955,  875,  885,  908,  857,  859,  867,  804,  590,  834,
    /*   390 */   747,  818,  513,  611,  741,  673,  637,  611,  606,  603,
    /*   400 */   579,  501,  541,  468,  386,  445,  395,  376,  281,  185,
    /*   410 */   120,   92,   75,   45,  114,   25,   11,    5,
};
#define YY_REDUCE_USE_DFLT (-169)
#define YY_REDUCE_COUNT (309)
#define YY_REDUCE_MIN   (-168)
#define YY_REDUCE_MAX   (1397)
static const short yy_reduce_ofst[] = {
    /*     0 */  -141,   90, 1095,  222,  158,  156,   19,   17,   10, -104,
    /*    10 */   378,  316,  311,   12,  180,  249,  598,  464,  397, 1181,
    /*    20 */  1177, 1175, 1128, 1106, 1096, 1054, 1038,  974,  964,  962,
    /*    30 */   948,  905,  903,  900,  887,  874,  832,  826,  816,  813,
    /*    40 */   800,  758,  755,  752,  742,  739,  726,  685,  681,  668,
    /*    50 */   665,  652,  607,  604,  594,  591,  578,  530,  528,  526,
    /*    60 */   385,   18,  477,  466,  519,  444,  350,  435,  405,  488,
    /*    70 */   488,  488,  488,  488,  488,  488,  488,  488,  488,  488,
    /*    80 */   488,  488,  488,  488,  488,  488,  488,  488,  488,  488,
    /*    90 */   488,  488,  488,  488,  488,  488,  488,  488,  488,  488,
    /*   100 */   488,  488,  488,  488,  488,  488,  488, 1040,  678, 1036,
    /*   110 */  1007,  967,  966,  965,  845,  686,  610,  684,  317,  672,
    /*   120 */   893,  327,  623,  522,   -7,  820,  814,  157,  154,  101,
    /*   130 */   702,  494,  580,  488,  488,  488,  488,  488,  614,  586,
    /*   140 */   935,  892,  968, 1245, 1242, 1234, 1225,  798,  798, 1222,
    /*   150 */  1221, 1218, 1214, 1213, 1212, 1202, 1195, 1191, 1161, 1158,
    /*   160 */  1140, 1135, 1123, 1112, 1107, 1100, 1080, 1074, 1073, 1072,
    /*   170 */  1070, 1067, 1048, 1044,  969,  968,  907,  906,  904,  894,
    /*   180 */   833,  837,  836,  340,  827,  815,  775,   68,  722,  646,
    /*   190 */  -168, 1389, 1381, 1371, 1379, 1373, 1370, 1343, 1352, 1369,
    /*   200 */  1352, 1352, 1352, 1352, 1352, 1352, 1352, 1325, 1320, 1352,
    /*   210 */  1352, 1343, 1380, 1353, 1397, 1351, 1339, 1334, 1319, 1341,
    /*   220 */  1303, 1364, 1359, 1368, 1362, 1366, 1360, 1350, 1354, 1318,
    /*   230 */  1313, 1307, 1305, 1363, 1328, 1324, 1372, 1278, 1361, 1358,
    /*   240 */  1277, 1276, 1356, 1296, 1322, 1309, 1317, 1315, 1314, 1312,
    /*   250 */  1345, 1347, 1302, 1292, 1311, 1304, 1293, 1337, 1335, 1252,
    /*   260 */  1248, 1332, 1330, 1329, 1327, 1326, 1323, 1321, 1297, 1301,
    /*   270 */  1295, 1294, 1290, 1243, 1240, 1284, 1291, 1286, 1283, 1274,
    /*   280 */  1281, 1271, 1238, 1241, 1236, 1235, 1227, 1226, 1267, 1266,
    /*   290 */  1189, 1229, 1223, 1211, 1206, 1201, 1197, 1239, 1237, 1219,
    /*   300 */  1216, 1209, 1208, 1185, 1089, 1086, 1087, 1137, 1136, 1164,
};
static const YYACTIONTYPE yy_default[] = {
    /*     0 */   633,  867,  955,  955,  867,  867,  955,  955,  955,  757,
    /*    10 */   955,  955,  955,  865,  955,  955,  785,  785,  929,  955,
    /*    20 */   955,  955,  955,  955,  955,  955,  955,  955,  955,  955,
    /*    30 */   955,  955,  955,  955,  955,  955,  955,  955,  955,  955,
    /*    40 */   955,  955,  955,  955,  955,  955,  955,  955,  955,  955,
    /*    50 */   955,  955,  955,  955,  955,  955,  955,  955,  955,  955,
    /*    60 */   955,  955,  955,  955,  955,  955,  955,  672,  761,  791,
    /*    70 */   955,  955,  955,  955,  955,  955,  955,  955,  928,  930,
    /*    80 */   799,  798,  908,  772,  796,  789,  793,  868,  861,  862,
    /*    90 */   860,  864,  869,  955,  792,  828,  845,  827,  839,  844,
    /*   100 */   851,  843,  840,  830,  829,  831,  832,  955,  955,  955,
    /*   110 */   955,  955,  955,  955,  955,  955,  955,  955,  955,  955,
    /*   120 */   955,  955,  955,  659,  726,  955,  955,  955,  955,  955,
    /*   130 */   955,  955,  955,  833,  834,  848,  847,  846,  955,  664,
    /*   140 */   955,  955,  955,  955,  955,  955,  955,  955,  955,  955,
    /*   150 */   935,  933,  955,  880,  955,  955,  955,  955,  955,  955,
    /*   160 */   955,  955,  955,  955,  955,  955,  955,  955,  955,  955,
    /*   170 */   955,  955,  955,  955,  955,  955,  955,  955,  955,  955,
    /*   180 */   639,  757,  757,  757,  633,  955,  955,  955,  947,  761,
    /*   190 */   751,  955,  955,  955,  955,  955,  955,  955,  955,  955,
    /*   200 */   955,  955,  955,  801,  740,  918,  920,  955,  901,  738,
    /*   210 */   661,  759,  674,  749,  641,  795,  774,  774,  913,  795,
    /*   220 */   913,  697,  720,  955,  785,  955,  785,  694,  785,  774,
    /*   230 */   863,  955,  955,  955,  758,  749,  955,  940,  765,  765,
    /*   240 */   932,  932,  765,  807,  730,  795,  737,  737,  737,  737,
    /*   250 */   765,  656,  795,  807,  730,  730,  795,  765,  656,  907,
    /*   260 */   905,  765,  765,  656,  765,  656,  765,  656,  873,  728,
    /*   270 */   728,  728,  712,  877,  877,  873,  728,  697,  728,  712,
    /*   280 */   728,  728,  778,  773,  778,  773,  778,  773,  765,  765,
    /*   290 */   955,  790,  779,  788,  786,  795,  955,  715,  649,  649,
    /*   300 */   638,  638,  638,  638,  952,  952,  947,  699,  699,  682,
    /*   310 */   955,  955,  955,  955,  955,  955,  955,  882,  955,  955,
    /*   320 */   955,  955,  955,  955,  955,  955,  955,  955,  955,  955,
    /*   330 */   634,  942,  955,  955,  939,  955,  955,  955,  955,  800,
    /*   340 */   955,  955,  955,  955,  955,  955,  955,  955,  955,  955,
    /*   350 */   917,  955,  955,  955,  955,  955,  955,  955,  911,  955,
    /*   360 */   955,  955,  955,  955,  955,  904,  903,  955,  955,  955,
    /*   370 */   955,  955,  955,  955,  955,  955,  955,  955,  955,  955,
    /*   380 */   955,  955,  955,  955,  955,  955,  955,  955,  955,  955,
    /*   390 */   955,  955,  955,  787,  955,  780,  955,  866,  955,  955,
    /*   400 */   955,  955,  955,  955,  955,  955,  955,  955,  743,  816,
    /*   410 */   955,  815,  819,  814,  666,  955,  647,  955,  630,  635,
    /*   420 */   951,  954,  953,  950,  949,  948,  943,  941,  938,  937,
    /*   430 */   936,  934,  931,  927,  886,  884,  891,  890,  889,  888,
    /*   440 */   887,  885,  883,  881,  802,  797,  794,  926,  879,  739,
    /*   450 */   736,  735,  655,  944,  910,  919,  806,  805,  808,  916,
    /*   460 */   915,  914,  912,  909,  896,  804,  803,  731,  871,  870,
    /*   470 */   658,  900,  899,  898,  902,  906,  897,  767,  657,  654,
    /*   480 */   663,  718,  719,  727,  725,  724,  723,  722,  721,  717,
    /*   490 */   665,  673,  711,  696,  695,  876,  878,  875,  874,  704,
    /*   500 */   703,  709,  708,  707,  706,  705,  702,  701,  700,  693,
    /*   510 */   692,  698,  691,  714,  713,  710,  690,  734,  733,  732,
    /*   520 */   729,  689,  688,  687,  819,  686,  685,  825,  824,  812,
    /*   530 */   855,  754,  753,  752,  764,  763,  776,  775,  810,  809,
    /*   540 */   777,  762,  756,  755,  771,  770,  769,  768,  760,  750,
    /*   550 */   782,  784,  783,  781,  857,  766,  854,  925,  924,  923,
    /*   560 */   922,  921,  859,  858,  826,  823,  677,  678,  894,  893,
    /*   570 */   895,  892,  680,  679,  676,  675,  856,  745,  744,  852,
    /*   580 */   849,  841,  837,  853,  850,  842,  838,  836,  835,  821,
    /*   590 */   820,  818,  817,  813,  822,  668,  746,  742,  741,  811,
    /*   600 */   748,  747,  684,  683,  681,  662,  660,  653,  651,  650,
    /*   610 */   652,  648,  646,  645,  644,  643,  642,  671,  670,  669,
    /*   620 */   667,  666,  640,  637,  636,  632,  631,  629,
};

/* The next table maps tokens into fallback tokens.  If a construct
 ** like the following:
 **
 **      %fallback ID X Y Z.
 **
 ** appears in the grammar, then ID becomes a fallback token for X, Y,
 ** and Z.  Whenever one of the tokens X, Y, or Z is input to the parser
 ** but it does not parse, the type of the token is changed to ID and
 ** the parse is retried before an error is thrown.
 */
#ifdef YYFALLBACK
static const YYCODETYPE yyFallback[] = {
    0,  /*          $ => nothing */
    0,  /*       SEMI => nothing */
    26,  /*    EXPLAIN => ID */
    26,  /*      QUERY => ID */
    26,  /*       PLAN => ID */
    26,  /*      BEGIN => ID */
    0,  /* TRANSACTION => nothing */
    26,  /*   DEFERRED => ID */
    26,  /*  IMMEDIATE => ID */
    26,  /*  EXCLUSIVE => ID */
    0,  /*     COMMIT => nothing */
    26,  /*        END => ID */
    26,  /*   ROLLBACK => ID */
    26,  /*  SAVEPOINT => ID */
    26,  /*    RELEASE => ID */
    0,  /*         TO => nothing */
    0,  /*      TABLE => nothing */
    0,  /*     CREATE => nothing */
    26,  /*         IF => ID */
    0,  /*        NOT => nothing */
    0,  /*     EXISTS => nothing */
    26,  /*       TEMP => ID */
    0,  /*         LP => nothing */
    0,  /*         RP => nothing */
    0,  /*         AS => nothing */
    0,  /*      COMMA => nothing */
    0,  /*         ID => nothing */
    0,  /*    INDEXED => nothing */
    26,  /*      ABORT => ID */
    26,  /*     ACTION => ID */
    26,  /*      AFTER => ID */
    26,  /*    ANALYZE => ID */
    26,  /*        ASC => ID */
    26,  /*     ATTACH => ID */
    26,  /*     BEFORE => ID */
    26,  /*         BY => ID */
    26,  /*    CASCADE => ID */
    26,  /*       CAST => ID */
    26,  /*   COLUMNKW => ID */
    26,  /*   CONFLICT => ID */
    26,  /*   DATABASE => ID */
    26,  /*       DESC => ID */
    26,  /*     DETACH => ID */
    26,  /*       EACH => ID */
    26,  /*       FAIL => ID */
    26,  /*        FOR => ID */
    26,  /*     IGNORE => ID */
    26,  /*  INITIALLY => ID */
    26,  /*    INSTEAD => ID */
    26,  /*    LIKE_KW => ID */
    26,  /*      MATCH => ID */
    26,  /*         NO => ID */
    26,  /*        KEY => ID */
    26,  /*         OF => ID */
    26,  /*     OFFSET => ID */
    26,  /*     PRAGMA => ID */
    26,  /*      RAISE => ID */
    26,  /*    REPLACE => ID */
    26,  /*   RESTRICT => ID */
    26,  /*        ROW => ID */
    26,  /*    TRIGGER => ID */
    26,  /*     VACUUM => ID */
    26,  /*       VIEW => ID */
    26,  /*    VIRTUAL => ID */
    26,  /*    REINDEX => ID */
    26,  /*     RENAME => ID */
    26,  /*   CTIME_KW => ID */
};
#endif /* YYFALLBACK */

/* The following structure represents a single element of the
 ** parser's stack.  Information stored includes:
 **
 **   +  The state number for the parser at this level of the stack.
 **
 **   +  The value of the token stored at this level of the stack.
 **      (In other words, the "major" token.)
 **
 **   +  The semantic value stored at this level of the stack.  This is
 **      the information used by the action routines in the grammar.
 **      It is sometimes called the "minor" token.
 */
struct yyStackEntry {
    YYACTIONTYPE stateno;  /* The state-number */
    YYCODETYPE major;      /* The major token value.  This is the code
                            ** number for the token at this stack level */
    YYMINORTYPE minor;     /* The user-supplied minor token value.  This
                            ** is the value of the token  */
};
typedef struct yyStackEntry yyStackEntry;

/* The state of the parser is completely contained in an instance of
 ** the following structure */
struct yyParser {
    int yyidx;                    /* Index of top element in stack */
#ifdef YYTRACKMAXSTACKDEPTH
    int yyidxMax;                 /* Maximum value of yyidx */
#endif
    int yyerrcnt;                 /* Shifts left before out of the error */
    sqlite3ParserARG_SDECL                /* A place to hold %extra_argument */
#if YYSTACKDEPTH<=0
    int yystksz;                  /* Current side of the stack */
    yyStackEntry *yystack;        /* The parser's stack */
#else
    yyStackEntry yystack[YYSTACKDEPTH];  /* The parser's stack */
#endif
};
typedef struct yyParser yyParser;

#ifndef NDEBUG
/* #include <stdio.h> */
static FILE *yyTraceFILE = 0;
static char *yyTracePrompt = 0;
#endif /* NDEBUG */

#ifndef NDEBUG
/*
 ** Turn parser tracing on by giving a stream to which to write the trace
 ** and a prompt to preface each trace message.  Tracing is turned off
 ** by making either argument NULL
 **
 ** Inputs:
 ** <ul>
 ** <li> A FILE* to which trace output should be written.
 **      If NULL, then tracing is turned off.
 ** <li> A prefix string written at the beginning of every
 **      line of trace output.  If NULL, then tracing is
 **      turned off.
 ** </ul>
 **
 ** Outputs:
 ** None.
 */
SQLITE_PRIVATE void sqlite3ParserTrace(FILE *TraceFILE, char *zTracePrompt){
    yyTraceFILE = TraceFILE;
    yyTracePrompt = zTracePrompt;
    if( yyTraceFILE==0 ) yyTracePrompt = 0;
    else if( yyTracePrompt==0 ) yyTraceFILE = 0;
}
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing shifts, the names of all terminals and nonterminals
 ** are required.  The following table supplies these names */
static const char *const yyTokenName[] = {
    "$",             "SEMI",          "EXPLAIN",       "QUERY",
    "PLAN",          "BEGIN",         "TRANSACTION",   "DEFERRED",
    "IMMEDIATE",     "EXCLUSIVE",     "COMMIT",        "END",
    "ROLLBACK",      "SAVEPOINT",     "RELEASE",       "TO",
    "TABLE",         "CREATE",        "IF",            "NOT",
    "EXISTS",        "TEMP",          "LP",            "RP",
    "AS",            "COMMA",         "ID",            "INDEXED",
    "ABORT",         "ACTION",        "AFTER",         "ANALYZE",
    "ASC",           "ATTACH",        "BEFORE",        "BY",
    "CASCADE",       "CAST",          "COLUMNKW",      "CONFLICT",
    "DATABASE",      "DESC",          "DETACH",        "EACH",
    "FAIL",          "FOR",           "IGNORE",        "INITIALLY",
    "INSTEAD",       "LIKE_KW",       "MATCH",         "NO",
    "KEY",           "OF",            "OFFSET",        "PRAGMA",
    "RAISE",         "REPLACE",       "RESTRICT",      "ROW",
    "TRIGGER",       "VACUUM",        "VIEW",          "VIRTUAL",
    "REINDEX",       "RENAME",        "CTIME_KW",      "ANY",
    "OR",            "AND",           "IS",            "BETWEEN",
    "IN",            "ISNULL",        "NOTNULL",       "NE",
    "EQ",            "GT",            "LE",            "LT",
    "GE",            "ESCAPE",        "BITAND",        "BITOR",
    "LSHIFT",        "RSHIFT",        "PLUS",          "MINUS",
    "STAR",          "SLASH",         "REM",           "CONCAT",
    "COLLATE",       "BITNOT",        "STRING",        "JOIN_KW",
    "CONSTRAINT",    "DEFAULT",       "NULL",          "PRIMARY",
    "UNIQUE",        "CHECK",         "REFERENCES",    "AUTOINCR",
    "ON",            "INSERT",        "DELETE",        "UPDATE",
    "SET",           "DEFERRABLE",    "FOREIGN",       "DROP",
    "UNION",         "ALL",           "EXCEPT",        "INTERSECT",
    "SELECT",        "DISTINCT",      "DOT",           "FROM",
    "JOIN",          "USING",         "ORDER",         "GROUP",
    "HAVING",        "LIMIT",         "WHERE",         "INTO",
    "VALUES",        "INTEGER",       "FLOAT",         "BLOB",
    "REGISTER",      "VARIABLE",      "CASE",          "WHEN",
    "THEN",          "ELSE",          "INDEX",         "ALTER",
    "ADD",           "error",         "input",         "cmdlist",
    "ecmd",          "explain",       "cmdx",          "cmd",
    "transtype",     "trans_opt",     "nm",            "savepoint_opt",
    "create_table",  "create_table_args",  "createkw",      "temp",
    "ifnotexists",   "dbnm",          "columnlist",    "conslist_opt",
    "select",        "column",        "columnid",      "type",
    "carglist",      "id",            "ids",           "typetoken",
    "typename",      "signed",        "plus_num",      "minus_num",
    "ccons",         "term",          "expr",          "onconf",
    "sortorder",     "autoinc",       "idxlist_opt",   "refargs",
    "defer_subclause",  "refarg",        "refact",        "init_deferred_pred_opt",
    "conslist",      "tconscomma",    "tcons",         "idxlist",
    "defer_subclause_opt",  "orconf",        "resolvetype",   "raisetype",
    "ifexists",      "fullname",      "oneselect",     "multiselect_op",
    "distinct",      "selcollist",    "from",          "where_opt",
    "groupby_opt",   "having_opt",    "orderby_opt",   "limit_opt",
    "sclp",          "as",            "seltablist",    "stl_prefix",
    "joinop",        "indexed_opt",   "on_opt",        "using_opt",
    "joinop2",       "inscollist",    "sortlist",      "nexprlist",
    "setlist",       "insert_cmd",    "inscollist_opt",  "valuelist",
    "exprlist",      "likeop",        "between_op",    "in_op",
    "case_operand",  "case_exprlist",  "case_else",     "uniqueflag",
    "collate",       "nmnum",         "number",        "trigger_decl",
    "trigger_cmd_list",  "trigger_time",  "trigger_event",  "foreach_clause",
    "when_clause",   "trigger_cmd",   "trnm",          "tridxby",
    "database_kw_opt",  "key_opt",       "add_column_fullname",  "kwcolumn_opt",
    "create_vtab",   "vtabarglist",   "vtabarg",       "vtabargtoken",
    "lp",            "anylist",
};
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing reduce actions, the names of all rules are required.
 */
static const char *const yyRuleName[] = {
    /*   0 */ "input ::= cmdlist",
    /*   1 */ "cmdlist ::= cmdlist ecmd",
    /*   2 */ "cmdlist ::= ecmd",
    /*   3 */ "ecmd ::= SEMI",
    /*   4 */ "ecmd ::= explain cmdx SEMI",
    /*   5 */ "explain ::=",
    /*   6 */ "explain ::= EXPLAIN",
    /*   7 */ "explain ::= EXPLAIN QUERY PLAN",
    /*   8 */ "cmdx ::= cmd",
    /*   9 */ "cmd ::= BEGIN transtype trans_opt",
    /*  10 */ "trans_opt ::=",
    /*  11 */ "trans_opt ::= TRANSACTION",
    /*  12 */ "trans_opt ::= TRANSACTION nm",
    /*  13 */ "transtype ::=",
    /*  14 */ "transtype ::= DEFERRED",
    /*  15 */ "transtype ::= IMMEDIATE",
    /*  16 */ "transtype ::= EXCLUSIVE",
    /*  17 */ "cmd ::= COMMIT trans_opt",
    /*  18 */ "cmd ::= END trans_opt",
    /*  19 */ "cmd ::= ROLLBACK trans_opt",
    /*  20 */ "savepoint_opt ::= SAVEPOINT",
    /*  21 */ "savepoint_opt ::=",
    /*  22 */ "cmd ::= SAVEPOINT nm",
    /*  23 */ "cmd ::= RELEASE savepoint_opt nm",
    /*  24 */ "cmd ::= ROLLBACK trans_opt TO savepoint_opt nm",
    /*  25 */ "cmd ::= create_table create_table_args",
    /*  26 */ "create_table ::= createkw temp TABLE ifnotexists nm dbnm",
    /*  27 */ "createkw ::= CREATE",
    /*  28 */ "ifnotexists ::=",
    /*  29 */ "ifnotexists ::= IF NOT EXISTS",
    /*  30 */ "temp ::= TEMP",
    /*  31 */ "temp ::=",
    /*  32 */ "create_table_args ::= LP columnlist conslist_opt RP",
    /*  33 */ "create_table_args ::= AS select",
    /*  34 */ "columnlist ::= columnlist COMMA column",
    /*  35 */ "columnlist ::= column",
    /*  36 */ "column ::= columnid type carglist",
    /*  37 */ "columnid ::= nm",
    /*  38 */ "id ::= ID",
    /*  39 */ "id ::= INDEXED",
    /*  40 */ "ids ::= ID|STRING",
    /*  41 */ "nm ::= id",
    /*  42 */ "nm ::= STRING",
    /*  43 */ "nm ::= JOIN_KW",
    /*  44 */ "type ::=",
    /*  45 */ "type ::= typetoken",
    /*  46 */ "typetoken ::= typename",
    /*  47 */ "typetoken ::= typename LP signed RP",
    /*  48 */ "typetoken ::= typename LP signed COMMA signed RP",
    /*  49 */ "typename ::= ids",
    /*  50 */ "typename ::= typename ids",
    /*  51 */ "signed ::= plus_num",
    /*  52 */ "signed ::= minus_num",
    /*  53 */ "carglist ::= carglist ccons",
    /*  54 */ "carglist ::=",
    /*  55 */ "ccons ::= CONSTRAINT nm",
    /*  56 */ "ccons ::= DEFAULT term",
    /*  57 */ "ccons ::= DEFAULT LP expr RP",
    /*  58 */ "ccons ::= DEFAULT PLUS term",
    /*  59 */ "ccons ::= DEFAULT MINUS term",
    /*  60 */ "ccons ::= DEFAULT id",
    /*  61 */ "ccons ::= NULL onconf",
    /*  62 */ "ccons ::= NOT NULL onconf",
    /*  63 */ "ccons ::= PRIMARY KEY sortorder onconf autoinc",
    /*  64 */ "ccons ::= UNIQUE onconf",
    /*  65 */ "ccons ::= CHECK LP expr RP",
    /*  66 */ "ccons ::= REFERENCES nm idxlist_opt refargs",
    /*  67 */ "ccons ::= defer_subclause",
    /*  68 */ "ccons ::= COLLATE ids",
    /*  69 */ "autoinc ::=",
    /*  70 */ "autoinc ::= AUTOINCR",
    /*  71 */ "refargs ::=",
    /*  72 */ "refargs ::= refargs refarg",
    /*  73 */ "refarg ::= MATCH nm",
    /*  74 */ "refarg ::= ON INSERT refact",
    /*  75 */ "refarg ::= ON DELETE refact",
    /*  76 */ "refarg ::= ON UPDATE refact",
    /*  77 */ "refact ::= SET NULL",
    /*  78 */ "refact ::= SET DEFAULT",
    /*  79 */ "refact ::= CASCADE",
    /*  80 */ "refact ::= RESTRICT",
    /*  81 */ "refact ::= NO ACTION",
    /*  82 */ "defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt",
    /*  83 */ "defer_subclause ::= DEFERRABLE init_deferred_pred_opt",
    /*  84 */ "init_deferred_pred_opt ::=",
    /*  85 */ "init_deferred_pred_opt ::= INITIALLY DEFERRED",
    /*  86 */ "init_deferred_pred_opt ::= INITIALLY IMMEDIATE",
    /*  87 */ "conslist_opt ::=",
    /*  88 */ "conslist_opt ::= COMMA conslist",
    /*  89 */ "conslist ::= conslist tconscomma tcons",
    /*  90 */ "conslist ::= tcons",
    /*  91 */ "tconscomma ::= COMMA",
    /*  92 */ "tconscomma ::=",
    /*  93 */ "tcons ::= CONSTRAINT nm",
    /*  94 */ "tcons ::= PRIMARY KEY LP idxlist autoinc RP onconf",
    /*  95 */ "tcons ::= UNIQUE LP idxlist RP onconf",
    /*  96 */ "tcons ::= CHECK LP expr RP onconf",
    /*  97 */ "tcons ::= FOREIGN KEY LP idxlist RP REFERENCES nm idxlist_opt refargs defer_subclause_opt",
    /*  98 */ "defer_subclause_opt ::=",
    /*  99 */ "defer_subclause_opt ::= defer_subclause",
    /* 100 */ "onconf ::=",
    /* 101 */ "onconf ::= ON CONFLICT resolvetype",
    /* 102 */ "orconf ::=",
    /* 103 */ "orconf ::= OR resolvetype",
    /* 104 */ "resolvetype ::= raisetype",
    /* 105 */ "resolvetype ::= IGNORE",
    /* 106 */ "resolvetype ::= REPLACE",
    /* 107 */ "cmd ::= DROP TABLE ifexists fullname",
    /* 108 */ "ifexists ::= IF EXISTS",
    /* 109 */ "ifexists ::=",
    /* 110 */ "cmd ::= createkw temp VIEW ifnotexists nm dbnm AS select",
    /* 111 */ "cmd ::= DROP VIEW ifexists fullname",
    /* 112 */ "cmd ::= select",
    /* 113 */ "select ::= oneselect",
    /* 114 */ "select ::= select multiselect_op oneselect",
    /* 115 */ "multiselect_op ::= UNION",
    /* 116 */ "multiselect_op ::= UNION ALL",
    /* 117 */ "multiselect_op ::= EXCEPT|INTERSECT",
    /* 118 */ "oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt orderby_opt limit_opt",
    /* 119 */ "distinct ::= DISTINCT",
    /* 120 */ "distinct ::= ALL",
    /* 121 */ "distinct ::=",
    /* 122 */ "sclp ::= selcollist COMMA",
    /* 123 */ "sclp ::=",
    /* 124 */ "selcollist ::= sclp expr as",
    /* 125 */ "selcollist ::= sclp STAR",
    /* 126 */ "selcollist ::= sclp nm DOT STAR",
    /* 127 */ "as ::= AS nm",
    /* 128 */ "as ::= ids",
    /* 129 */ "as ::=",
    /* 130 */ "from ::=",
    /* 131 */ "from ::= FROM seltablist",
    /* 132 */ "stl_prefix ::= seltablist joinop",
    /* 133 */ "stl_prefix ::=",
    /* 134 */ "seltablist ::= stl_prefix nm dbnm as indexed_opt on_opt using_opt",
    /* 135 */ "seltablist ::= stl_prefix LP select RP as on_opt using_opt",
    /* 136 */ "seltablist ::= stl_prefix LP seltablist RP as on_opt using_opt",
    /* 137 */ "dbnm ::=",
    /* 138 */ "dbnm ::= DOT nm",
    /* 139 */ "fullname ::= nm dbnm",
    /* 140 */ "joinop ::= COMMA|JOIN",
    /* 141 */ "joinop ::= JOIN_KW JOIN",
    /* 142 */ "joinop ::= JOIN_KW nm JOIN",
    /* 143 */ "joinop ::= JOIN_KW nm nm JOIN",
    /* 144 */ "on_opt ::= ON expr",
    /* 145 */ "on_opt ::=",
    /* 146 */ "indexed_opt ::=",
    /* 147 */ "indexed_opt ::= INDEXED BY nm",
    /* 148 */ "indexed_opt ::= NOT INDEXED",
    /* 149 */ "using_opt ::= USING LP inscollist RP",
    /* 150 */ "using_opt ::=",
    /* 151 */ "orderby_opt ::=",
    /* 152 */ "orderby_opt ::= ORDER BY sortlist",
    /* 153 */ "sortlist ::= sortlist COMMA expr sortorder",
    /* 154 */ "sortlist ::= expr sortorder",
    /* 155 */ "sortorder ::= ASC",
    /* 156 */ "sortorder ::= DESC",
    /* 157 */ "sortorder ::=",
    /* 158 */ "groupby_opt ::=",
    /* 159 */ "groupby_opt ::= GROUP BY nexprlist",
    /* 160 */ "having_opt ::=",
    /* 161 */ "having_opt ::= HAVING expr",
    /* 162 */ "limit_opt ::=",
    /* 163 */ "limit_opt ::= LIMIT expr",
    /* 164 */ "limit_opt ::= LIMIT expr OFFSET expr",
    /* 165 */ "limit_opt ::= LIMIT expr COMMA expr",
    /* 166 */ "cmd ::= DELETE FROM fullname indexed_opt where_opt",
    /* 167 */ "where_opt ::=",
    /* 168 */ "where_opt ::= WHERE expr",
    /* 169 */ "cmd ::= UPDATE orconf fullname indexed_opt SET setlist where_opt",
    /* 170 */ "setlist ::= setlist COMMA nm EQ expr",
    /* 171 */ "setlist ::= nm EQ expr",
    /* 172 */ "cmd ::= insert_cmd INTO fullname inscollist_opt valuelist",
    /* 173 */ "cmd ::= insert_cmd INTO fullname inscollist_opt select",
    /* 174 */ "cmd ::= insert_cmd INTO fullname inscollist_opt DEFAULT VALUES",
    /* 175 */ "insert_cmd ::= INSERT orconf",
    /* 176 */ "insert_cmd ::= REPLACE",
    /* 177 */ "valuelist ::= VALUES LP nexprlist RP",
    /* 178 */ "valuelist ::= valuelist COMMA LP exprlist RP",
    /* 179 */ "inscollist_opt ::=",
    /* 180 */ "inscollist_opt ::= LP inscollist RP",
    /* 181 */ "inscollist ::= inscollist COMMA nm",
    /* 182 */ "inscollist ::= nm",
    /* 183 */ "expr ::= term",
    /* 184 */ "expr ::= LP expr RP",
    /* 185 */ "term ::= NULL",
    /* 186 */ "expr ::= id",
    /* 187 */ "expr ::= JOIN_KW",
    /* 188 */ "expr ::= nm DOT nm",
    /* 189 */ "expr ::= nm DOT nm DOT nm",
    /* 190 */ "term ::= INTEGER|FLOAT|BLOB",
    /* 191 */ "term ::= STRING",
    /* 192 */ "expr ::= REGISTER",
    /* 193 */ "expr ::= VARIABLE",
    /* 194 */ "expr ::= expr COLLATE ids",
    /* 195 */ "expr ::= CAST LP expr AS typetoken RP",
    /* 196 */ "expr ::= ID LP distinct exprlist RP",
    /* 197 */ "expr ::= ID LP STAR RP",
    /* 198 */ "term ::= CTIME_KW",
    /* 199 */ "expr ::= expr AND expr",
    /* 200 */ "expr ::= expr OR expr",
    /* 201 */ "expr ::= expr LT|GT|GE|LE expr",
    /* 202 */ "expr ::= expr EQ|NE expr",
    /* 203 */ "expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr",
    /* 204 */ "expr ::= expr PLUS|MINUS expr",
    /* 205 */ "expr ::= expr STAR|SLASH|REM expr",
    /* 206 */ "expr ::= expr CONCAT expr",
    /* 207 */ "likeop ::= LIKE_KW",
    /* 208 */ "likeop ::= NOT LIKE_KW",
    /* 209 */ "likeop ::= MATCH",
    /* 210 */ "likeop ::= NOT MATCH",
    /* 211 */ "expr ::= expr likeop expr",
    /* 212 */ "expr ::= expr likeop expr ESCAPE expr",
    /* 213 */ "expr ::= expr ISNULL|NOTNULL",
    /* 214 */ "expr ::= expr NOT NULL",
    /* 215 */ "expr ::= expr IS expr",
    /* 216 */ "expr ::= expr IS NOT expr",
    /* 217 */ "expr ::= NOT expr",
    /* 218 */ "expr ::= BITNOT expr",
    /* 219 */ "expr ::= MINUS expr",
    /* 220 */ "expr ::= PLUS expr",
    /* 221 */ "between_op ::= BETWEEN",
    /* 222 */ "between_op ::= NOT BETWEEN",
    /* 223 */ "expr ::= expr between_op expr AND expr",
    /* 224 */ "in_op ::= IN",
    /* 225 */ "in_op ::= NOT IN",
    /* 226 */ "expr ::= expr in_op LP exprlist RP",
    /* 227 */ "expr ::= LP select RP",
    /* 228 */ "expr ::= expr in_op LP select RP",
    /* 229 */ "expr ::= expr in_op nm dbnm",
    /* 230 */ "expr ::= EXISTS LP select RP",
    /* 231 */ "expr ::= CASE case_operand case_exprlist case_else END",
    /* 232 */ "case_exprlist ::= case_exprlist WHEN expr THEN expr",
    /* 233 */ "case_exprlist ::= WHEN expr THEN expr",
    /* 234 */ "case_else ::= ELSE expr",
    /* 235 */ "case_else ::=",
    /* 236 */ "case_operand ::= expr",
    /* 237 */ "case_operand ::=",
    /* 238 */ "exprlist ::= nexprlist",
    /* 239 */ "exprlist ::=",
    /* 240 */ "nexprlist ::= nexprlist COMMA expr",
    /* 241 */ "nexprlist ::= expr",
    /* 242 */ "cmd ::= createkw uniqueflag INDEX ifnotexists nm dbnm ON nm LP idxlist RP where_opt",
    /* 243 */ "uniqueflag ::= UNIQUE",
    /* 244 */ "uniqueflag ::=",
    /* 245 */ "idxlist_opt ::=",
    /* 246 */ "idxlist_opt ::= LP idxlist RP",
    /* 247 */ "idxlist ::= idxlist COMMA nm collate sortorder",
    /* 248 */ "idxlist ::= nm collate sortorder",
    /* 249 */ "collate ::=",
    /* 250 */ "collate ::= COLLATE ids",
    /* 251 */ "cmd ::= DROP INDEX ifexists fullname",
    /* 252 */ "cmd ::= VACUUM",
    /* 253 */ "cmd ::= VACUUM nm",
    /* 254 */ "cmd ::= PRAGMA nm dbnm",
    /* 255 */ "cmd ::= PRAGMA nm dbnm EQ nmnum",
    /* 256 */ "cmd ::= PRAGMA nm dbnm LP nmnum RP",
    /* 257 */ "cmd ::= PRAGMA nm dbnm EQ minus_num",
    /* 258 */ "cmd ::= PRAGMA nm dbnm LP minus_num RP",
    /* 259 */ "nmnum ::= plus_num",
    /* 260 */ "nmnum ::= nm",
    /* 261 */ "nmnum ::= ON",
    /* 262 */ "nmnum ::= DELETE",
    /* 263 */ "nmnum ::= DEFAULT",
    /* 264 */ "plus_num ::= PLUS number",
    /* 265 */ "plus_num ::= number",
    /* 266 */ "minus_num ::= MINUS number",
    /* 267 */ "number ::= INTEGER|FLOAT",
    /* 268 */ "cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END",
    /* 269 */ "trigger_decl ::= temp TRIGGER ifnotexists nm dbnm trigger_time trigger_event ON fullname foreach_clause when_clause",
    /* 270 */ "trigger_time ::= BEFORE",
    /* 271 */ "trigger_time ::= AFTER",
    /* 272 */ "trigger_time ::= INSTEAD OF",
    /* 273 */ "trigger_time ::=",
    /* 274 */ "trigger_event ::= DELETE|INSERT",
    /* 275 */ "trigger_event ::= UPDATE",
    /* 276 */ "trigger_event ::= UPDATE OF inscollist",
    /* 277 */ "foreach_clause ::=",
    /* 278 */ "foreach_clause ::= FOR EACH ROW",
    /* 279 */ "when_clause ::=",
    /* 280 */ "when_clause ::= WHEN expr",
    /* 281 */ "trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI",
    /* 282 */ "trigger_cmd_list ::= trigger_cmd SEMI",
    /* 283 */ "trnm ::= nm",
    /* 284 */ "trnm ::= nm DOT nm",
    /* 285 */ "tridxby ::=",
    /* 286 */ "tridxby ::= INDEXED BY nm",
    /* 287 */ "tridxby ::= NOT INDEXED",
    /* 288 */ "trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist where_opt",
    /* 289 */ "trigger_cmd ::= insert_cmd INTO trnm inscollist_opt valuelist",
    /* 290 */ "trigger_cmd ::= insert_cmd INTO trnm inscollist_opt select",
    /* 291 */ "trigger_cmd ::= DELETE FROM trnm tridxby where_opt",
    /* 292 */ "trigger_cmd ::= select",
    /* 293 */ "expr ::= RAISE LP IGNORE RP",
    /* 294 */ "expr ::= RAISE LP raisetype COMMA nm RP",
    /* 295 */ "raisetype ::= ROLLBACK",
    /* 296 */ "raisetype ::= ABORT",
    /* 297 */ "raisetype ::= FAIL",
    /* 298 */ "cmd ::= DROP TRIGGER ifexists fullname",
    /* 299 */ "cmd ::= ATTACH database_kw_opt expr AS expr key_opt",
    /* 300 */ "cmd ::= DETACH database_kw_opt expr",
    /* 301 */ "key_opt ::=",
    /* 302 */ "key_opt ::= KEY expr",
    /* 303 */ "database_kw_opt ::= DATABASE",
    /* 304 */ "database_kw_opt ::=",
    /* 305 */ "cmd ::= REINDEX",
    /* 306 */ "cmd ::= REINDEX nm dbnm",
    /* 307 */ "cmd ::= ANALYZE",
    /* 308 */ "cmd ::= ANALYZE nm dbnm",
    /* 309 */ "cmd ::= ALTER TABLE fullname RENAME TO nm",
    /* 310 */ "cmd ::= ALTER TABLE add_column_fullname ADD kwcolumn_opt column",
    /* 311 */ "add_column_fullname ::= fullname",
    /* 312 */ "kwcolumn_opt ::=",
    /* 313 */ "kwcolumn_opt ::= COLUMNKW",
    /* 314 */ "cmd ::= create_vtab",
    /* 315 */ "cmd ::= create_vtab LP vtabarglist RP",
    /* 316 */ "create_vtab ::= createkw VIRTUAL TABLE ifnotexists nm dbnm USING nm",
    /* 317 */ "vtabarglist ::= vtabarg",
    /* 318 */ "vtabarglist ::= vtabarglist COMMA vtabarg",
    /* 319 */ "vtabarg ::=",
    /* 320 */ "vtabarg ::= vtabarg vtabargtoken",
    /* 321 */ "vtabargtoken ::= ANY",
    /* 322 */ "vtabargtoken ::= lp anylist RP",
    /* 323 */ "lp ::= LP",
    /* 324 */ "anylist ::=",
    /* 325 */ "anylist ::= anylist LP anylist RP",
    /* 326 */ "anylist ::= anylist ANY",
};
#endif /* NDEBUG */


#if YYSTACKDEPTH<=0
/*
 ** Try to increase the size of the parser stack.
 */
static void yyGrowStack(yyParser *p){
    int newSize;
    yyStackEntry *pNew;
    
    newSize = p->yystksz*2 + 100;
    pNew = realloc(p->yystack, newSize*sizeof(pNew[0]));
    if( pNew ){
        p->yystack = pNew;
        p->yystksz = newSize;
#ifndef NDEBUG
        if( yyTraceFILE ){
            fprintf(yyTraceFILE,"%sStack grows to %d entries!\n",
                    yyTracePrompt, p->yystksz);
        }
#endif
    }
}
#endif

/*
 ** This function allocates a new parser.
 ** The only argument is a pointer to a function which works like
 ** malloc.
 **
 ** Inputs:
 ** A pointer to the function used to allocate memory.
 **
 ** Outputs:
 ** A pointer to a parser.  This pointer is used in subsequent calls
 ** to sqlite3Parser and sqlite3ParserFree.
 */
SQLITE_PRIVATE void *sqlite3ParserAlloc(void *(*mallocProc)(size_t)){
    yyParser *pParser;
    pParser = (yyParser*)(*mallocProc)( (size_t)sizeof(yyParser) );
    if( pParser ){
        pParser->yyidx = -1;
#ifdef YYTRACKMAXSTACKDEPTH
        pParser->yyidxMax = 0;
#endif
#if YYSTACKDEPTH<=0
        pParser->yystack = NULL;
        pParser->yystksz = 0;
        yyGrowStack(pParser);
#endif
    }
    return pParser;
}

/* The following function deletes the value associated with a
 ** symbol.  The symbol can be either a terminal or nonterminal.
 ** "yymajor" is the symbol code, and "yypminor" is a pointer to
 ** the value.
 */
static void yy_destructor(
                          yyParser *yypParser,    /* The parser */
                          YYCODETYPE yymajor,     /* Type code for object to destroy */
                          YYMINORTYPE *yypminor   /* The object to be destroyed */
){
    sqlite3ParserARG_FETCH;
    switch( yymajor ){
            /* Here is inserted the actions which take place when a
             ** terminal or non-terminal is destroyed.  This can happen
             ** when the symbol is popped from the stack during a
             ** reduce or during error processing or when a parser is
             ** being destroyed before it is finished parsing.
             **
             ** Note: during a reduce, the only symbols destroyed are those
             ** which appear on the RHS of the rule, but which are not used
             ** inside the C code.
             */
        case 160: /* select */
        case 194: /* oneselect */
        {
            sqlite3SelectDelete(pParse->db, (yypminor->yy159));
        }
            break;
        case 173: /* term */
        case 174: /* expr */
        {
            sqlite3ExprDelete(pParse->db, (yypminor->yy342).pExpr);
        }
            break;
        case 178: /* idxlist_opt */
        case 187: /* idxlist */
        case 197: /* selcollist */
        case 200: /* groupby_opt */
        case 202: /* orderby_opt */
        case 204: /* sclp */
        case 214: /* sortlist */
        case 215: /* nexprlist */
        case 216: /* setlist */
        case 220: /* exprlist */
        case 225: /* case_exprlist */
        {
            sqlite3ExprListDelete(pParse->db, (yypminor->yy442));
        }
            break;
        case 193: /* fullname */
        case 198: /* from */
        case 206: /* seltablist */
        case 207: /* stl_prefix */
        {
            sqlite3SrcListDelete(pParse->db, (yypminor->yy347));
        }
            break;
        case 199: /* where_opt */
        case 201: /* having_opt */
        case 210: /* on_opt */
        case 224: /* case_operand */
        case 226: /* case_else */
        case 236: /* when_clause */
        case 241: /* key_opt */
        {
            sqlite3ExprDelete(pParse->db, (yypminor->yy122));
        }
            break;
        case 211: /* using_opt */
        case 213: /* inscollist */
        case 218: /* inscollist_opt */
        {
            sqlite3IdListDelete(pParse->db, (yypminor->yy180));
        }
            break;
        case 219: /* valuelist */
        {
            
            sqlite3ExprListDelete(pParse->db, (yypminor->yy487).pList);
            sqlite3SelectDelete(pParse->db, (yypminor->yy487).pSelect);
            
        }
            break;
        case 232: /* trigger_cmd_list */
        case 237: /* trigger_cmd */
        {
            sqlite3DeleteTriggerStep(pParse->db, (yypminor->yy327));
        }
            break;
        case 234: /* trigger_event */
        {
            sqlite3IdListDelete(pParse->db, (yypminor->yy410).b);
        }
            break;
        default:  break;   /* If no destructor action specified: do nothing */
    }
}

/*
 ** Pop the parser's stack once.
 **
 ** If there is a destructor routine associated with the token which
 ** is popped from the stack, then call it.
 **
 ** Return the major token number for the symbol popped.
 */
static int yy_pop_parser_stack(yyParser *pParser){
    YYCODETYPE yymajor;
    yyStackEntry *yytos = &pParser->yystack[pParser->yyidx];
    
    /* There is no mechanism by which the parser stack can be popped below
     ** empty in SQLite.  */
    if( NEVER(pParser->yyidx<0) ) return 0;
#ifndef NDEBUG
    if( yyTraceFILE && pParser->yyidx>=0 ){
        fprintf(yyTraceFILE,"%sPopping %s\n",
                yyTracePrompt,
                yyTokenName[yytos->major]);
    }
#endif
    yymajor = yytos->major;
    yy_destructor(pParser, yymajor, &yytos->minor);
    pParser->yyidx--;
    return yymajor;
}

/*
 ** Deallocate and destroy a parser.  Destructors are all called for
 ** all stack elements before shutting the parser down.
 **
 ** Inputs:
 ** <ul>
 ** <li>  A pointer to the parser.  This should be a pointer
 **       obtained from sqlite3ParserAlloc.
 ** <li>  A pointer to a function used to reclaim memory obtained
 **       from malloc.
 ** </ul>
 */
SQLITE_PRIVATE void sqlite3ParserFree(
                                      void *p,                    /* The parser to be deleted */
                                      void (*freeProc)(void*)     /* Function used to reclaim memory */
){
    yyParser *pParser = (yyParser*)p;
    /* In SQLite, we never try to destroy a parser that was not successfully
     ** created in the first place. */
    if( NEVER(pParser==0) ) return;
    while( pParser->yyidx>=0 ) yy_pop_parser_stack(pParser);
#if YYSTACKDEPTH<=0
    free(pParser->yystack);
#endif
    (*freeProc)((void*)pParser);
}

/*
 ** Return the peak depth of the stack for a parser.
 */
#ifdef YYTRACKMAXSTACKDEPTH
SQLITE_PRIVATE int sqlite3ParserStackPeak(void *p){
    yyParser *pParser = (yyParser*)p;
    return pParser->yyidxMax;
}
#endif

/*
 ** Find the appropriate action for a parser given the terminal
 ** look-ahead token iLookAhead.
 **
 ** If the look-ahead token is YYNOCODE, then check to see if the action is
 ** independent of the look-ahead.  If it is, return the action, otherwise
 ** return YY_NO_ACTION.
 */
static int yy_find_shift_action(
                                yyParser *pParser,        /* The parser */
                                YYCODETYPE iLookAhead     /* The look-ahead token */
){
    int i;
    int stateno = pParser->yystack[pParser->yyidx].stateno;
    
    if( stateno>YY_SHIFT_COUNT
       || (i = yy_shift_ofst[stateno])==YY_SHIFT_USE_DFLT ){
        return yy_default[stateno];
    }
    assert( iLookAhead!=YYNOCODE );
    i += iLookAhead;
    if( i<0 || i>=YY_ACTTAB_COUNT || yy_lookahead[i]!=iLookAhead ){
        if( iLookAhead>0 ){
#ifdef YYFALLBACK
            YYCODETYPE iFallback;            /* Fallback token */
            if( iLookAhead<sizeof(yyFallback)/sizeof(yyFallback[0])
               && (iFallback = yyFallback[iLookAhead])!=0 ){
#ifndef NDEBUG
                if( yyTraceFILE ){
                    fprintf(yyTraceFILE, "%sFALLBACK %s => %s\n",
                            yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[iFallback]);
                }
#endif
                return yy_find_shift_action(pParser, iFallback);
            }
#endif
#ifdef YYWILDCARD
            {
                int j = i - iLookAhead + YYWILDCARD;
                if(
#if YY_SHIFT_MIN+YYWILDCARD<0
                   j>=0 &&
#endif
#if YY_SHIFT_MAX+YYWILDCARD>=YY_ACTTAB_COUNT
                   j<YY_ACTTAB_COUNT &&
#endif
                   yy_lookahead[j]==YYWILDCARD
                   ){
#ifndef NDEBUG
                    if( yyTraceFILE ){
                        fprintf(yyTraceFILE, "%sWILDCARD %s => %s\n",
                                yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[YYWILDCARD]);
                    }
#endif /* NDEBUG */
                    return yy_action[j];
                }
            }
#endif /* YYWILDCARD */
        }
        return yy_default[stateno];
    }else{
        return yy_action[i];
    }
}

/*
 ** Find the appropriate action for a parser given the non-terminal
 ** look-ahead token iLookAhead.
 **
 ** If the look-ahead token is YYNOCODE, then check to see if the action is
 ** independent of the look-ahead.  If it is, return the action, otherwise
 ** return YY_NO_ACTION.
 */
static int yy_find_reduce_action(
                                 int stateno,              /* Current state number */
                                 YYCODETYPE iLookAhead     /* The look-ahead token */
){
    int i;
#ifdef YYERRORSYMBOL
    if( stateno>YY_REDUCE_COUNT ){
        return yy_default[stateno];
    }
#else
    assert( stateno<=YY_REDUCE_COUNT );
#endif
    i = yy_reduce_ofst[stateno];
    assert( i!=YY_REDUCE_USE_DFLT );
    assert( iLookAhead!=YYNOCODE );
    i += iLookAhead;
#ifdef YYERRORSYMBOL
    if( i<0 || i>=YY_ACTTAB_COUNT || yy_lookahead[i]!=iLookAhead ){
        return yy_default[stateno];
    }
#else
    assert( i>=0 && i<YY_ACTTAB_COUNT );
    assert( yy_lookahead[i]==iLookAhead );
#endif
    return yy_action[i];
}

/*
 ** The following routine is called if the stack overflows.
 */
static void yyStackOverflow(yyParser *yypParser, YYMINORTYPE *yypMinor){
    sqlite3ParserARG_FETCH;
    yypParser->yyidx--;
#ifndef NDEBUG
    if( yyTraceFILE ){
        fprintf(yyTraceFILE,"%sStack Overflow!\n",yyTracePrompt);
    }
#endif
    while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
    /* Here code is inserted which will execute if the parser
     ** stack every overflows */
    
    UNUSED_PARAMETER(yypMinor); /* Silence some compiler warnings */
    sqlite3ErrorMsg(pParse, "parser stack overflow");
    sqlite3ParserARG_STORE; /* Suppress warning about unused %extra_argument var */
}

/*
 ** Perform a shift action.
 */
static void yy_shift(
                     yyParser *yypParser,          /* The parser to be shifted */
                     int yyNewState,               /* The new state to shift in */
                     int yyMajor,                  /* The major token to shift in */
                     YYMINORTYPE *yypMinor         /* Pointer to the minor token to shift in */
){
    yyStackEntry *yytos;
    yypParser->yyidx++;
#ifdef YYTRACKMAXSTACKDEPTH
    if( yypParser->yyidx>yypParser->yyidxMax ){
        yypParser->yyidxMax = yypParser->yyidx;
    }
#endif
#if YYSTACKDEPTH>0
    if( yypParser->yyidx>=YYSTACKDEPTH ){
        yyStackOverflow(yypParser, yypMinor);
        return;
    }
#else
    if( yypParser->yyidx>=yypParser->yystksz ){
        yyGrowStack(yypParser);
        if( yypParser->yyidx>=yypParser->yystksz ){
            yyStackOverflow(yypParser, yypMinor);
            return;
        }
    }
#endif
    yytos = &yypParser->yystack[yypParser->yyidx];
    yytos->stateno = (YYACTIONTYPE)yyNewState;
    yytos->major = (YYCODETYPE)yyMajor;
    yytos->minor = *yypMinor;
#ifndef NDEBUG
    if( yyTraceFILE && yypParser->yyidx>0 ){
        int i;
        fprintf(yyTraceFILE,"%sShift %d\n",yyTracePrompt,yyNewState);
        fprintf(yyTraceFILE,"%sStack:",yyTracePrompt);
        for(i=1; i<=yypParser->yyidx; i++)
            fprintf(yyTraceFILE," %s",yyTokenName[yypParser->yystack[i].major]);
        fprintf(yyTraceFILE,"\n");
    }
#endif
}

/* The following table contains information about every rule that
 ** is used during the reduce.
 */
static const struct {
    YYCODETYPE lhs;         /* Symbol on the left-hand side of the rule */
    unsigned char nrhs;     /* Number of right-hand side symbols in the rule */
} yyRuleInfo[] = {
    { 142, 1 },
    { 143, 2 },
    { 143, 1 },
    { 144, 1 },
    { 144, 3 },
    { 145, 0 },
    { 145, 1 },
    { 145, 3 },
    { 146, 1 },
    { 147, 3 },
    { 149, 0 },
    { 149, 1 },
    { 149, 2 },
    { 148, 0 },
    { 148, 1 },
    { 148, 1 },
    { 148, 1 },
    { 147, 2 },
    { 147, 2 },
    { 147, 2 },
    { 151, 1 },
    { 151, 0 },
    { 147, 2 },
    { 147, 3 },
    { 147, 5 },
    { 147, 2 },
    { 152, 6 },
    { 154, 1 },
    { 156, 0 },
    { 156, 3 },
    { 155, 1 },
    { 155, 0 },
    { 153, 4 },
    { 153, 2 },
    { 158, 3 },
    { 158, 1 },
    { 161, 3 },
    { 162, 1 },
    { 165, 1 },
    { 165, 1 },
    { 166, 1 },
    { 150, 1 },
    { 150, 1 },
    { 150, 1 },
    { 163, 0 },
    { 163, 1 },
    { 167, 1 },
    { 167, 4 },
    { 167, 6 },
    { 168, 1 },
    { 168, 2 },
    { 169, 1 },
    { 169, 1 },
    { 164, 2 },
    { 164, 0 },
    { 172, 2 },
    { 172, 2 },
    { 172, 4 },
    { 172, 3 },
    { 172, 3 },
    { 172, 2 },
    { 172, 2 },
    { 172, 3 },
    { 172, 5 },
    { 172, 2 },
    { 172, 4 },
    { 172, 4 },
    { 172, 1 },
    { 172, 2 },
    { 177, 0 },
    { 177, 1 },
    { 179, 0 },
    { 179, 2 },
    { 181, 2 },
    { 181, 3 },
    { 181, 3 },
    { 181, 3 },
    { 182, 2 },
    { 182, 2 },
    { 182, 1 },
    { 182, 1 },
    { 182, 2 },
    { 180, 3 },
    { 180, 2 },
    { 183, 0 },
    { 183, 2 },
    { 183, 2 },
    { 159, 0 },
    { 159, 2 },
    { 184, 3 },
    { 184, 1 },
    { 185, 1 },
    { 185, 0 },
    { 186, 2 },
    { 186, 7 },
    { 186, 5 },
    { 186, 5 },
    { 186, 10 },
    { 188, 0 },
    { 188, 1 },
    { 175, 0 },
    { 175, 3 },
    { 189, 0 },
    { 189, 2 },
    { 190, 1 },
    { 190, 1 },
    { 190, 1 },
    { 147, 4 },
    { 192, 2 },
    { 192, 0 },
    { 147, 8 },
    { 147, 4 },
    { 147, 1 },
    { 160, 1 },
    { 160, 3 },
    { 195, 1 },
    { 195, 2 },
    { 195, 1 },
    { 194, 9 },
    { 196, 1 },
    { 196, 1 },
    { 196, 0 },
    { 204, 2 },
    { 204, 0 },
    { 197, 3 },
    { 197, 2 },
    { 197, 4 },
    { 205, 2 },
    { 205, 1 },
    { 205, 0 },
    { 198, 0 },
    { 198, 2 },
    { 207, 2 },
    { 207, 0 },
    { 206, 7 },
    { 206, 7 },
    { 206, 7 },
    { 157, 0 },
    { 157, 2 },
    { 193, 2 },
    { 208, 1 },
    { 208, 2 },
    { 208, 3 },
    { 208, 4 },
    { 210, 2 },
    { 210, 0 },
    { 209, 0 },
    { 209, 3 },
    { 209, 2 },
    { 211, 4 },
    { 211, 0 },
    { 202, 0 },
    { 202, 3 },
    { 214, 4 },
    { 214, 2 },
    { 176, 1 },
    { 176, 1 },
    { 176, 0 },
    { 200, 0 },
    { 200, 3 },
    { 201, 0 },
    { 201, 2 },
    { 203, 0 },
    { 203, 2 },
    { 203, 4 },
    { 203, 4 },
    { 147, 5 },
    { 199, 0 },
    { 199, 2 },
    { 147, 7 },
    { 216, 5 },
    { 216, 3 },
    { 147, 5 },
    { 147, 5 },
    { 147, 6 },
    { 217, 2 },
    { 217, 1 },
    { 219, 4 },
    { 219, 5 },
    { 218, 0 },
    { 218, 3 },
    { 213, 3 },
    { 213, 1 },
    { 174, 1 },
    { 174, 3 },
    { 173, 1 },
    { 174, 1 },
    { 174, 1 },
    { 174, 3 },
    { 174, 5 },
    { 173, 1 },
    { 173, 1 },
    { 174, 1 },
    { 174, 1 },
    { 174, 3 },
    { 174, 6 },
    { 174, 5 },
    { 174, 4 },
    { 173, 1 },
    { 174, 3 },
    { 174, 3 },
    { 174, 3 },
    { 174, 3 },
    { 174, 3 },
    { 174, 3 },
    { 174, 3 },
    { 174, 3 },
    { 221, 1 },
    { 221, 2 },
    { 221, 1 },
    { 221, 2 },
    { 174, 3 },
    { 174, 5 },
    { 174, 2 },
    { 174, 3 },
    { 174, 3 },
    { 174, 4 },
    { 174, 2 },
    { 174, 2 },
    { 174, 2 },
    { 174, 2 },
    { 222, 1 },
    { 222, 2 },
    { 174, 5 },
    { 223, 1 },
    { 223, 2 },
    { 174, 5 },
    { 174, 3 },
    { 174, 5 },
    { 174, 4 },
    { 174, 4 },
    { 174, 5 },
    { 225, 5 },
    { 225, 4 },
    { 226, 2 },
    { 226, 0 },
    { 224, 1 },
    { 224, 0 },
    { 220, 1 },
    { 220, 0 },
    { 215, 3 },
    { 215, 1 },
    { 147, 12 },
    { 227, 1 },
    { 227, 0 },
    { 178, 0 },
    { 178, 3 },
    { 187, 5 },
    { 187, 3 },
    { 228, 0 },
    { 228, 2 },
    { 147, 4 },
    { 147, 1 },
    { 147, 2 },
    { 147, 3 },
    { 147, 5 },
    { 147, 6 },
    { 147, 5 },
    { 147, 6 },
    { 229, 1 },
    { 229, 1 },
    { 229, 1 },
    { 229, 1 },
    { 229, 1 },
    { 170, 2 },
    { 170, 1 },
    { 171, 2 },
    { 230, 1 },
    { 147, 5 },
    { 231, 11 },
    { 233, 1 },
    { 233, 1 },
    { 233, 2 },
    { 233, 0 },
    { 234, 1 },
    { 234, 1 },
    { 234, 3 },
    { 235, 0 },
    { 235, 3 },
    { 236, 0 },
    { 236, 2 },
    { 232, 3 },
    { 232, 2 },
    { 238, 1 },
    { 238, 3 },
    { 239, 0 },
    { 239, 3 },
    { 239, 2 },
    { 237, 7 },
    { 237, 5 },
    { 237, 5 },
    { 237, 5 },
    { 237, 1 },
    { 174, 4 },
    { 174, 6 },
    { 191, 1 },
    { 191, 1 },
    { 191, 1 },
    { 147, 4 },
    { 147, 6 },
    { 147, 3 },
    { 241, 0 },
    { 241, 2 },
    { 240, 1 },
    { 240, 0 },
    { 147, 1 },
    { 147, 3 },
    { 147, 1 },
    { 147, 3 },
    { 147, 6 },
    { 147, 6 },
    { 242, 1 },
    { 243, 0 },
    { 243, 1 },
    { 147, 1 },
    { 147, 4 },
    { 244, 8 },
    { 245, 1 },
    { 245, 3 },
    { 246, 0 },
    { 246, 2 },
    { 247, 1 },
    { 247, 3 },
    { 248, 1 },
    { 249, 0 },
    { 249, 4 },
    { 249, 2 },
};

static void yy_accept(yyParser*);  /* Forward Declaration */

/*
 ** Perform a reduce action and the shift that must immediately
 ** follow the reduce.
 */
static void yy_reduce(
                      yyParser *yypParser,         /* The parser */
                      int yyruleno                 /* Number of the rule by which to reduce */
){
    int yygoto;                     /* The next state */
    int yyact;                      /* The next action */
    YYMINORTYPE yygotominor;        /* The LHS of the rule reduced */
    yyStackEntry *yymsp;            /* The top of the parser's stack */
    int yysize;                     /* Amount to pop the stack */
    sqlite3ParserARG_FETCH;
    yymsp = &yypParser->yystack[yypParser->yyidx];
#ifndef NDEBUG
    if( yyTraceFILE && yyruleno>=0
       && yyruleno<(int)(sizeof(yyRuleName)/sizeof(yyRuleName[0])) ){
        fprintf(yyTraceFILE, "%sReduce [%s].\n", yyTracePrompt,
                yyRuleName[yyruleno]);
    }
#endif /* NDEBUG */
    
    /* Silence complaints from purify about yygotominor being uninitialized
     ** in some cases when it is copied into the stack after the following
     ** switch.  yygotominor is uninitialized when a rule reduces that does
     ** not set the value of its left-hand side nonterminal.  Leaving the
     ** value of the nonterminal uninitialized is utterly harmless as long
     ** as the value is never used.  So really the only thing this code
     ** accomplishes is to quieten purify.
     **
     ** 2007-01-16:  The wireshark project (www.wireshark.org) reports that
     ** without this code, their parser segfaults.  I'm not sure what there
     ** parser is doing to make this happen.  This is the second bug report
     ** from wireshark this week.  Clearly they are stressing Lemon in ways
     ** that it has not been previously stressed...  (SQLite ticket #2172)
     */
    /*memset(&yygotominor, 0, sizeof(yygotominor));*/
    yygotominor = yyzerominor;
    
    
    switch( yyruleno ){
            /* Beginning here are the reduction cases.  A typical example
             ** follows:
             **   case 0:
             **  #line <lineno> <grammarfile>
             **     { ... }           // User supplied code
             **  #line <lineno> <thisfile>
             **     break;
             */
        case 5: /* explain ::= */
        { sqlite3BeginParse(pParse, 0); }
            break;
        case 6: /* explain ::= EXPLAIN */
        { sqlite3BeginParse(pParse, 1); }
            break;
        case 7: /* explain ::= EXPLAIN QUERY PLAN */
        { sqlite3BeginParse(pParse, 2); }
            break;
        case 8: /* cmdx ::= cmd */
        { sqlite3FinishCoding(pParse); }
            break;
        case 9: /* cmd ::= BEGIN transtype trans_opt */
        {sqlite3BeginTransaction(pParse, yymsp[-1].minor.yy392);}
            break;
        case 13: /* transtype ::= */
        {yygotominor.yy392 = TK_DEFERRED;}
            break;
        case 14: /* transtype ::= DEFERRED */
        case 15: /* transtype ::= IMMEDIATE */ yytestcase(yyruleno==15);
        case 16: /* transtype ::= EXCLUSIVE */ yytestcase(yyruleno==16);
        case 115: /* multiselect_op ::= UNION */ yytestcase(yyruleno==115);
        case 117: /* multiselect_op ::= EXCEPT|INTERSECT */ yytestcase(yyruleno==117);
        {yygotominor.yy392 = yymsp[0].major;}
            break;
        case 17: /* cmd ::= COMMIT trans_opt */
        case 18: /* cmd ::= END trans_opt */ yytestcase(yyruleno==18);
        {sqlite3CommitTransaction(pParse);}
            break;
        case 19: /* cmd ::= ROLLBACK trans_opt */
        {sqlite3RollbackTransaction(pParse);}
            break;
        case 22: /* cmd ::= SAVEPOINT nm */
        {
            sqlite3Savepoint(pParse, SAVEPOINT_BEGIN, &yymsp[0].minor.yy0);
        }
            break;
        case 23: /* cmd ::= RELEASE savepoint_opt nm */
        {
            sqlite3Savepoint(pParse, SAVEPOINT_RELEASE, &yymsp[0].minor.yy0);
        }
            break;
        case 24: /* cmd ::= ROLLBACK trans_opt TO savepoint_opt nm */
        {
            sqlite3Savepoint(pParse, SAVEPOINT_ROLLBACK, &yymsp[0].minor.yy0);
        }
            break;
        case 26: /* create_table ::= createkw temp TABLE ifnotexists nm dbnm */
        {
            sqlite3StartTable(pParse,&yymsp[-1].minor.yy0,&yymsp[0].minor.yy0,yymsp[-4].minor.yy392,0,0,yymsp[-2].minor.yy392);
        }
            break;
        case 27: /* createkw ::= CREATE */
        {
            pParse->db->lookaside.bEnabled = 0;
            yygotominor.yy0 = yymsp[0].minor.yy0;
        }
            break;
        case 28: /* ifnotexists ::= */
        case 31: /* temp ::= */ yytestcase(yyruleno==31);
        case 69: /* autoinc ::= */ yytestcase(yyruleno==69);
        case 82: /* defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt */ yytestcase(yyruleno==82);
        case 84: /* init_deferred_pred_opt ::= */ yytestcase(yyruleno==84);
        case 86: /* init_deferred_pred_opt ::= INITIALLY IMMEDIATE */ yytestcase(yyruleno==86);
        case 98: /* defer_subclause_opt ::= */ yytestcase(yyruleno==98);
        case 109: /* ifexists ::= */ yytestcase(yyruleno==109);
        case 221: /* between_op ::= BETWEEN */ yytestcase(yyruleno==221);
        case 224: /* in_op ::= IN */ yytestcase(yyruleno==224);
        {yygotominor.yy392 = 0;}
            break;
        case 29: /* ifnotexists ::= IF NOT EXISTS */
        case 30: /* temp ::= TEMP */ yytestcase(yyruleno==30);
        case 70: /* autoinc ::= AUTOINCR */ yytestcase(yyruleno==70);
        case 85: /* init_deferred_pred_opt ::= INITIALLY DEFERRED */ yytestcase(yyruleno==85);
        case 108: /* ifexists ::= IF EXISTS */ yytestcase(yyruleno==108);
        case 222: /* between_op ::= NOT BETWEEN */ yytestcase(yyruleno==222);
        case 225: /* in_op ::= NOT IN */ yytestcase(yyruleno==225);
        {yygotominor.yy392 = 1;}
            break;
        case 32: /* create_table_args ::= LP columnlist conslist_opt RP */
        {
            sqlite3EndTable(pParse,&yymsp[-1].minor.yy0,&yymsp[0].minor.yy0,0);
        }
            break;
        case 33: /* create_table_args ::= AS select */
        {
            sqlite3EndTable(pParse,0,0,yymsp[0].minor.yy159);
            sqlite3SelectDelete(pParse->db, yymsp[0].minor.yy159);
        }
            break;
        case 36: /* column ::= columnid type carglist */
        {
            yygotominor.yy0.z = yymsp[-2].minor.yy0.z;
            yygotominor.yy0.n = (int)(pParse->sLastToken.z-yymsp[-2].minor.yy0.z) + pParse->sLastToken.n;
        }
            break;
        case 37: /* columnid ::= nm */
        {
            sqlite3AddColumn(pParse,&yymsp[0].minor.yy0);
            yygotominor.yy0 = yymsp[0].minor.yy0;
            pParse->constraintName.n = 0;
        }
            break;
        case 38: /* id ::= ID */
        case 39: /* id ::= INDEXED */ yytestcase(yyruleno==39);
        case 40: /* ids ::= ID|STRING */ yytestcase(yyruleno==40);
        case 41: /* nm ::= id */ yytestcase(yyruleno==41);
        case 42: /* nm ::= STRING */ yytestcase(yyruleno==42);
        case 43: /* nm ::= JOIN_KW */ yytestcase(yyruleno==43);
        case 46: /* typetoken ::= typename */ yytestcase(yyruleno==46);
        case 49: /* typename ::= ids */ yytestcase(yyruleno==49);
        case 127: /* as ::= AS nm */ yytestcase(yyruleno==127);
        case 128: /* as ::= ids */ yytestcase(yyruleno==128);
        case 138: /* dbnm ::= DOT nm */ yytestcase(yyruleno==138);
        case 147: /* indexed_opt ::= INDEXED BY nm */ yytestcase(yyruleno==147);
        case 250: /* collate ::= COLLATE ids */ yytestcase(yyruleno==250);
        case 259: /* nmnum ::= plus_num */ yytestcase(yyruleno==259);
        case 260: /* nmnum ::= nm */ yytestcase(yyruleno==260);
        case 261: /* nmnum ::= ON */ yytestcase(yyruleno==261);
        case 262: /* nmnum ::= DELETE */ yytestcase(yyruleno==262);
        case 263: /* nmnum ::= DEFAULT */ yytestcase(yyruleno==263);
        case 264: /* plus_num ::= PLUS number */ yytestcase(yyruleno==264);
        case 265: /* plus_num ::= number */ yytestcase(yyruleno==265);
        case 266: /* minus_num ::= MINUS number */ yytestcase(yyruleno==266);
        case 267: /* number ::= INTEGER|FLOAT */ yytestcase(yyruleno==267);
        case 283: /* trnm ::= nm */ yytestcase(yyruleno==283);
        {yygotominor.yy0 = yymsp[0].minor.yy0;}
            break;
        case 45: /* type ::= typetoken */
        {sqlite3AddColumnType(pParse,&yymsp[0].minor.yy0);}
            break;
        case 47: /* typetoken ::= typename LP signed RP */
        {
            yygotominor.yy0.z = yymsp[-3].minor.yy0.z;
            yygotominor.yy0.n = (int)(&yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n] - yymsp[-3].minor.yy0.z);
        }
            break;
        case 48: /* typetoken ::= typename LP signed COMMA signed RP */
        {
            yygotominor.yy0.z = yymsp[-5].minor.yy0.z;
            yygotominor.yy0.n = (int)(&yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n] - yymsp[-5].minor.yy0.z);
        }
            break;
        case 50: /* typename ::= typename ids */
        {yygotominor.yy0.z=yymsp[-1].minor.yy0.z; yygotominor.yy0.n=yymsp[0].minor.yy0.n+(int)(yymsp[0].minor.yy0.z-yymsp[-1].minor.yy0.z);}
            break;
        case 55: /* ccons ::= CONSTRAINT nm */
        case 93: /* tcons ::= CONSTRAINT nm */ yytestcase(yyruleno==93);
        {pParse->constraintName = yymsp[0].minor.yy0;}
            break;
        case 56: /* ccons ::= DEFAULT term */
        case 58: /* ccons ::= DEFAULT PLUS term */ yytestcase(yyruleno==58);
        {sqlite3AddDefaultValue(pParse,&yymsp[0].minor.yy342);}
            break;
        case 57: /* ccons ::= DEFAULT LP expr RP */
        {sqlite3AddDefaultValue(pParse,&yymsp[-1].minor.yy342);}
            break;
        case 59: /* ccons ::= DEFAULT MINUS term */
        {
            ExprSpan v;
            v.pExpr = sqlite3PExpr(pParse, TK_UMINUS, yymsp[0].minor.yy342.pExpr, 0, 0);
            v.zStart = yymsp[-1].minor.yy0.z;
            v.zEnd = yymsp[0].minor.yy342.zEnd;
            sqlite3AddDefaultValue(pParse,&v);
        }
            break;
        case 60: /* ccons ::= DEFAULT id */
        {
            ExprSpan v;
            spanExpr(&v, pParse, TK_STRING, &yymsp[0].minor.yy0);
            sqlite3AddDefaultValue(pParse,&v);
        }
            break;
        case 62: /* ccons ::= NOT NULL onconf */
        {sqlite3AddNotNull(pParse, yymsp[0].minor.yy392);}
            break;
        case 63: /* ccons ::= PRIMARY KEY sortorder onconf autoinc */
        {sqlite3AddPrimaryKey(pParse,0,yymsp[-1].minor.yy392,yymsp[0].minor.yy392,yymsp[-2].minor.yy392);}
            break;
        case 64: /* ccons ::= UNIQUE onconf */
        {sqlite3CreateIndex(pParse,0,0,0,0,yymsp[0].minor.yy392,0,0,0,0);}
            break;
        case 65: /* ccons ::= CHECK LP expr RP */
        {sqlite3AddCheckConstraint(pParse,yymsp[-1].minor.yy342.pExpr);}
            break;
        case 66: /* ccons ::= REFERENCES nm idxlist_opt refargs */
        {sqlite3CreateForeignKey(pParse,0,&yymsp[-2].minor.yy0,yymsp[-1].minor.yy442,yymsp[0].minor.yy392);}
            break;
        case 67: /* ccons ::= defer_subclause */
        {sqlite3DeferForeignKey(pParse,yymsp[0].minor.yy392);}
            break;
        case 68: /* ccons ::= COLLATE ids */
        {sqlite3AddCollateType(pParse, &yymsp[0].minor.yy0);}
            break;
        case 71: /* refargs ::= */
        { yygotominor.yy392 = OE_None*0x0101; /* EV: R-19803-45884 */}
            break;
        case 72: /* refargs ::= refargs refarg */
        { yygotominor.yy392 = (yymsp[-1].minor.yy392 & ~yymsp[0].minor.yy207.mask) | yymsp[0].minor.yy207.value; }
            break;
        case 73: /* refarg ::= MATCH nm */
        case 74: /* refarg ::= ON INSERT refact */ yytestcase(yyruleno==74);
        { yygotominor.yy207.value = 0;     yygotominor.yy207.mask = 0x000000; }
            break;
        case 75: /* refarg ::= ON DELETE refact */
        { yygotominor.yy207.value = yymsp[0].minor.yy392;     yygotominor.yy207.mask = 0x0000ff; }
            break;
        case 76: /* refarg ::= ON UPDATE refact */
        { yygotominor.yy207.value = yymsp[0].minor.yy392<<8;  yygotominor.yy207.mask = 0x00ff00; }
            break;
        case 77: /* refact ::= SET NULL */
        { yygotominor.yy392 = OE_SetNull;  /* EV: R-33326-45252 */}
            break;
        case 78: /* refact ::= SET DEFAULT */
        { yygotominor.yy392 = OE_SetDflt;  /* EV: R-33326-45252 */}
            break;
        case 79: /* refact ::= CASCADE */
        { yygotominor.yy392 = OE_Cascade;  /* EV: R-33326-45252 */}
            break;
        case 80: /* refact ::= RESTRICT */
        { yygotominor.yy392 = OE_Restrict; /* EV: R-33326-45252 */}
            break;
        case 81: /* refact ::= NO ACTION */
        { yygotominor.yy392 = OE_None;     /* EV: R-33326-45252 */}
            break;
        case 83: /* defer_subclause ::= DEFERRABLE init_deferred_pred_opt */
        case 99: /* defer_subclause_opt ::= defer_subclause */ yytestcase(yyruleno==99);
        case 101: /* onconf ::= ON CONFLICT resolvetype */ yytestcase(yyruleno==101);
        case 104: /* resolvetype ::= raisetype */ yytestcase(yyruleno==104);
        {yygotominor.yy392 = yymsp[0].minor.yy392;}
            break;
        case 87: /* conslist_opt ::= */
        {yygotominor.yy0.n = 0; yygotominor.yy0.z = 0;}
            break;
        case 88: /* conslist_opt ::= COMMA conslist */
        {yygotominor.yy0 = yymsp[-1].minor.yy0;}
            break;
        case 91: /* tconscomma ::= COMMA */
        {pParse->constraintName.n = 0;}
            break;
        case 94: /* tcons ::= PRIMARY KEY LP idxlist autoinc RP onconf */
        {sqlite3AddPrimaryKey(pParse,yymsp[-3].minor.yy442,yymsp[0].minor.yy392,yymsp[-2].minor.yy392,0);}
            break;
        case 95: /* tcons ::= UNIQUE LP idxlist RP onconf */
        {sqlite3CreateIndex(pParse,0,0,0,yymsp[-2].minor.yy442,yymsp[0].minor.yy392,0,0,0,0);}
            break;
        case 96: /* tcons ::= CHECK LP expr RP onconf */
        {sqlite3AddCheckConstraint(pParse,yymsp[-2].minor.yy342.pExpr);}
            break;
        case 97: /* tcons ::= FOREIGN KEY LP idxlist RP REFERENCES nm idxlist_opt refargs defer_subclause_opt */
        {
            sqlite3CreateForeignKey(pParse, yymsp[-6].minor.yy442, &yymsp[-3].minor.yy0, yymsp[-2].minor.yy442, yymsp[-1].minor.yy392);
            sqlite3DeferForeignKey(pParse, yymsp[0].minor.yy392);
        }
            break;
        case 100: /* onconf ::= */
        {yygotominor.yy392 = OE_Default;}
            break;
        case 102: /* orconf ::= */
        {yygotominor.yy258 = OE_Default;}
            break;
        case 103: /* orconf ::= OR resolvetype */
        {yygotominor.yy258 = (u8)yymsp[0].minor.yy392;}
            break;
        case 105: /* resolvetype ::= IGNORE */
        {yygotominor.yy392 = OE_Ignore;}
            break;
        case 106: /* resolvetype ::= REPLACE */
        {yygotominor.yy392 = OE_Replace;}
            break;
        case 107: /* cmd ::= DROP TABLE ifexists fullname */
        {
            sqlite3DropTable(pParse, yymsp[0].minor.yy347, 0, yymsp[-1].minor.yy392);
        }
            break;
        case 110: /* cmd ::= createkw temp VIEW ifnotexists nm dbnm AS select */
        {
            sqlite3CreateView(pParse, &yymsp[-7].minor.yy0, &yymsp[-3].minor.yy0, &yymsp[-2].minor.yy0, yymsp[0].minor.yy159, yymsp[-6].minor.yy392, yymsp[-4].minor.yy392);
        }
            break;
        case 111: /* cmd ::= DROP VIEW ifexists fullname */
        {
            sqlite3DropTable(pParse, yymsp[0].minor.yy347, 1, yymsp[-1].minor.yy392);
        }
            break;
        case 112: /* cmd ::= select */
        {
            SelectDest dest = {SRT_Output, 0, 0, 0, 0};
            sqlite3Select(pParse, yymsp[0].minor.yy159, &dest);
            sqlite3ExplainBegin(pParse->pVdbe);
            sqlite3ExplainSelect(pParse->pVdbe, yymsp[0].minor.yy159);
            sqlite3ExplainFinish(pParse->pVdbe);
            sqlite3SelectDelete(pParse->db, yymsp[0].minor.yy159);
        }
            break;
        case 113: /* select ::= oneselect */
        {yygotominor.yy159 = yymsp[0].minor.yy159;}
            break;
        case 114: /* select ::= select multiselect_op oneselect */
        {
            if( yymsp[0].minor.yy159 ){
                yymsp[0].minor.yy159->op = (u8)yymsp[-1].minor.yy392;
                yymsp[0].minor.yy159->pPrior = yymsp[-2].minor.yy159;
                if( yymsp[-1].minor.yy392!=TK_ALL ) pParse->hasCompound = 1;
            }else{
                sqlite3SelectDelete(pParse->db, yymsp[-2].minor.yy159);
            }
            yygotominor.yy159 = yymsp[0].minor.yy159;
        }
            break;
        case 116: /* multiselect_op ::= UNION ALL */
        {yygotominor.yy392 = TK_ALL;}
            break;
        case 118: /* oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt orderby_opt limit_opt */
        {
            yygotominor.yy159 = sqlite3SelectNew(pParse,yymsp[-6].minor.yy442,yymsp[-5].minor.yy347,yymsp[-4].minor.yy122,yymsp[-3].minor.yy442,yymsp[-2].minor.yy122,yymsp[-1].minor.yy442,yymsp[-7].minor.yy305,yymsp[0].minor.yy64.pLimit,yymsp[0].minor.yy64.pOffset);
        }
            break;
        case 119: /* distinct ::= DISTINCT */
        {yygotominor.yy305 = SF_Distinct;}
            break;
        case 120: /* distinct ::= ALL */
        case 121: /* distinct ::= */ yytestcase(yyruleno==121);
        {yygotominor.yy305 = 0;}
            break;
        case 122: /* sclp ::= selcollist COMMA */
        case 246: /* idxlist_opt ::= LP idxlist RP */ yytestcase(yyruleno==246);
        {yygotominor.yy442 = yymsp[-1].minor.yy442;}
            break;
        case 123: /* sclp ::= */
        case 151: /* orderby_opt ::= */ yytestcase(yyruleno==151);
        case 158: /* groupby_opt ::= */ yytestcase(yyruleno==158);
        case 239: /* exprlist ::= */ yytestcase(yyruleno==239);
        case 245: /* idxlist_opt ::= */ yytestcase(yyruleno==245);
        {yygotominor.yy442 = 0;}
            break;
        case 124: /* selcollist ::= sclp expr as */
        {
            yygotominor.yy442 = sqlite3ExprListAppend(pParse, yymsp[-2].minor.yy442, yymsp[-1].minor.yy342.pExpr);
            if( yymsp[0].minor.yy0.n>0 ) sqlite3ExprListSetName(pParse, yygotominor.yy442, &yymsp[0].minor.yy0, 1);
            sqlite3ExprListSetSpan(pParse,yygotominor.yy442,&yymsp[-1].minor.yy342);
        }
            break;
        case 125: /* selcollist ::= sclp STAR */
        {
            Expr *p = sqlite3Expr(pParse->db, TK_ALL, 0);
            yygotominor.yy442 = sqlite3ExprListAppend(pParse, yymsp[-1].minor.yy442, p);
        }
            break;
        case 126: /* selcollist ::= sclp nm DOT STAR */
        {
            Expr *pRight = sqlite3PExpr(pParse, TK_ALL, 0, 0, &yymsp[0].minor.yy0);
            Expr *pLeft = sqlite3PExpr(pParse, TK_ID, 0, 0, &yymsp[-2].minor.yy0);
            Expr *pDot = sqlite3PExpr(pParse, TK_DOT, pLeft, pRight, 0);
            yygotominor.yy442 = sqlite3ExprListAppend(pParse,yymsp[-3].minor.yy442, pDot);
        }
            break;
        case 129: /* as ::= */
        {yygotominor.yy0.n = 0;}
            break;
        case 130: /* from ::= */
        {yygotominor.yy347 = sqlite3DbMallocZero(pParse->db, sizeof(*yygotominor.yy347));}
            break;
        case 131: /* from ::= FROM seltablist */
        {
            yygotominor.yy347 = yymsp[0].minor.yy347;
            sqlite3SrcListShiftJoinType(yygotominor.yy347);
        }
            break;
        case 132: /* stl_prefix ::= seltablist joinop */
        {
            yygotominor.yy347 = yymsp[-1].minor.yy347;
            if( ALWAYS(yygotominor.yy347 && yygotominor.yy347->nSrc>0) ) yygotominor.yy347->a[yygotominor.yy347->nSrc-1].jointype = (u8)yymsp[0].minor.yy392;
        }
            break;
        case 133: /* stl_prefix ::= */
        {yygotominor.yy347 = 0;}
            break;
        case 134: /* seltablist ::= stl_prefix nm dbnm as indexed_opt on_opt using_opt */
        {
            yygotominor.yy347 = sqlite3SrcListAppendFromTerm(pParse,yymsp[-6].minor.yy347,&yymsp[-5].minor.yy0,&yymsp[-4].minor.yy0,&yymsp[-3].minor.yy0,0,yymsp[-1].minor.yy122,yymsp[0].minor.yy180);
            sqlite3SrcListIndexedBy(pParse, yygotominor.yy347, &yymsp[-2].minor.yy0);
        }
            break;
        case 135: /* seltablist ::= stl_prefix LP select RP as on_opt using_opt */
        {
            yygotominor.yy347 = sqlite3SrcListAppendFromTerm(pParse,yymsp[-6].minor.yy347,0,0,&yymsp[-2].minor.yy0,yymsp[-4].minor.yy159,yymsp[-1].minor.yy122,yymsp[0].minor.yy180);
        }
            break;
        case 136: /* seltablist ::= stl_prefix LP seltablist RP as on_opt using_opt */
        {
            if( yymsp[-6].minor.yy347==0 && yymsp[-2].minor.yy0.n==0 && yymsp[-1].minor.yy122==0 && yymsp[0].minor.yy180==0 ){
                yygotominor.yy347 = yymsp[-4].minor.yy347;
            }else if( yymsp[-4].minor.yy347->nSrc==1 ){
                yygotominor.yy347 = sqlite3SrcListAppendFromTerm(pParse,yymsp[-6].minor.yy347,0,0,&yymsp[-2].minor.yy0,0,yymsp[-1].minor.yy122,yymsp[0].minor.yy180);
                if( yygotominor.yy347 ){
                    struct SrcList_item *pNew = &yygotominor.yy347->a[yygotominor.yy347->nSrc-1];
                    struct SrcList_item *pOld = yymsp[-4].minor.yy347->a;
                    pNew->zName = pOld->zName;
                    pNew->zDatabase = pOld->zDatabase;
                    pNew->pSelect = pOld->pSelect;
                    pOld->zName = pOld->zDatabase = 0;
                    pOld->pSelect = 0;
                }
                sqlite3SrcListDelete(pParse->db, yymsp[-4].minor.yy347);
            }else{
                Select *pSubquery;
                sqlite3SrcListShiftJoinType(yymsp[-4].minor.yy347);
                pSubquery = sqlite3SelectNew(pParse,0,yymsp[-4].minor.yy347,0,0,0,0,SF_NestedFrom,0,0);
                yygotominor.yy347 = sqlite3SrcListAppendFromTerm(pParse,yymsp[-6].minor.yy347,0,0,&yymsp[-2].minor.yy0,pSubquery,yymsp[-1].minor.yy122,yymsp[0].minor.yy180);
            }
        }
            break;
        case 137: /* dbnm ::= */
        case 146: /* indexed_opt ::= */ yytestcase(yyruleno==146);
        {yygotominor.yy0.z=0; yygotominor.yy0.n=0;}
            break;
        case 139: /* fullname ::= nm dbnm */
        {yygotominor.yy347 = sqlite3SrcListAppend(pParse->db,0,&yymsp[-1].minor.yy0,&yymsp[0].minor.yy0);}
            break;
        case 140: /* joinop ::= COMMA|JOIN */
        { yygotominor.yy392 = JT_INNER; }
            break;
        case 141: /* joinop ::= JOIN_KW JOIN */
        { yygotominor.yy392 = sqlite3JoinType(pParse,&yymsp[-1].minor.yy0,0,0); }
            break;
        case 142: /* joinop ::= JOIN_KW nm JOIN */
        { yygotominor.yy392 = sqlite3JoinType(pParse,&yymsp[-2].minor.yy0,&yymsp[-1].minor.yy0,0); }
            break;
        case 143: /* joinop ::= JOIN_KW nm nm JOIN */
        { yygotominor.yy392 = sqlite3JoinType(pParse,&yymsp[-3].minor.yy0,&yymsp[-2].minor.yy0,&yymsp[-1].minor.yy0); }
            break;
        case 144: /* on_opt ::= ON expr */
        case 161: /* having_opt ::= HAVING expr */ yytestcase(yyruleno==161);
        case 168: /* where_opt ::= WHERE expr */ yytestcase(yyruleno==168);
        case 234: /* case_else ::= ELSE expr */ yytestcase(yyruleno==234);
        case 236: /* case_operand ::= expr */ yytestcase(yyruleno==236);
        {yygotominor.yy122 = yymsp[0].minor.yy342.pExpr;}
            break;
        case 145: /* on_opt ::= */
        case 160: /* having_opt ::= */ yytestcase(yyruleno==160);
        case 167: /* where_opt ::= */ yytestcase(yyruleno==167);
        case 235: /* case_else ::= */ yytestcase(yyruleno==235);
        case 237: /* case_operand ::= */ yytestcase(yyruleno==237);
        {yygotominor.yy122 = 0;}
            break;
        case 148: /* indexed_opt ::= NOT INDEXED */
        {yygotominor.yy0.z=0; yygotominor.yy0.n=1;}
            break;
        case 149: /* using_opt ::= USING LP inscollist RP */
        case 180: /* inscollist_opt ::= LP inscollist RP */ yytestcase(yyruleno==180);
        {yygotominor.yy180 = yymsp[-1].minor.yy180;}
            break;
        case 150: /* using_opt ::= */
        case 179: /* inscollist_opt ::= */ yytestcase(yyruleno==179);
        {yygotominor.yy180 = 0;}
            break;
        case 152: /* orderby_opt ::= ORDER BY sortlist */
        case 159: /* groupby_opt ::= GROUP BY nexprlist */ yytestcase(yyruleno==159);
        case 238: /* exprlist ::= nexprlist */ yytestcase(yyruleno==238);
        {yygotominor.yy442 = yymsp[0].minor.yy442;}
            break;
        case 153: /* sortlist ::= sortlist COMMA expr sortorder */
        {
            yygotominor.yy442 = sqlite3ExprListAppend(pParse,yymsp[-3].minor.yy442,yymsp[-1].minor.yy342.pExpr);
            if( yygotominor.yy442 ) yygotominor.yy442->a[yygotominor.yy442->nExpr-1].sortOrder = (u8)yymsp[0].minor.yy392;
        }
            break;
        case 154: /* sortlist ::= expr sortorder */
        {
            yygotominor.yy442 = sqlite3ExprListAppend(pParse,0,yymsp[-1].minor.yy342.pExpr);
            if( yygotominor.yy442 && ALWAYS(yygotominor.yy442->a) ) yygotominor.yy442->a[0].sortOrder = (u8)yymsp[0].minor.yy392;
        }
            break;
        case 155: /* sortorder ::= ASC */
        case 157: /* sortorder ::= */ yytestcase(yyruleno==157);
        {yygotominor.yy392 = SQLITE_SO_ASC;}
            break;
        case 156: /* sortorder ::= DESC */
        {yygotominor.yy392 = SQLITE_SO_DESC;}
            break;
        case 162: /* limit_opt ::= */
        {yygotominor.yy64.pLimit = 0; yygotominor.yy64.pOffset = 0;}
            break;
        case 163: /* limit_opt ::= LIMIT expr */
        {yygotominor.yy64.pLimit = yymsp[0].minor.yy342.pExpr; yygotominor.yy64.pOffset = 0;}
            break;
        case 164: /* limit_opt ::= LIMIT expr OFFSET expr */
        {yygotominor.yy64.pLimit = yymsp[-2].minor.yy342.pExpr; yygotominor.yy64.pOffset = yymsp[0].minor.yy342.pExpr;}
            break;
        case 165: /* limit_opt ::= LIMIT expr COMMA expr */
        {yygotominor.yy64.pOffset = yymsp[-2].minor.yy342.pExpr; yygotominor.yy64.pLimit = yymsp[0].minor.yy342.pExpr;}
            break;
        case 166: /* cmd ::= DELETE FROM fullname indexed_opt where_opt */
        {
            sqlite3SrcListIndexedBy(pParse, yymsp[-2].minor.yy347, &yymsp[-1].minor.yy0);
            sqlite3DeleteFrom(pParse,yymsp[-2].minor.yy347,yymsp[0].minor.yy122);
        }
            break;
        case 169: /* cmd ::= UPDATE orconf fullname indexed_opt SET setlist where_opt */
        {
            sqlite3SrcListIndexedBy(pParse, yymsp[-4].minor.yy347, &yymsp[-3].minor.yy0);
            sqlite3ExprListCheckLength(pParse,yymsp[-1].minor.yy442,"set list");
            sqlite3Update(pParse,yymsp[-4].minor.yy347,yymsp[-1].minor.yy442,yymsp[0].minor.yy122,yymsp[-5].minor.yy258);
        }
            break;
        case 170: /* setlist ::= setlist COMMA nm EQ expr */
        {
            yygotominor.yy442 = sqlite3ExprListAppend(pParse, yymsp[-4].minor.yy442, yymsp[0].minor.yy342.pExpr);
            sqlite3ExprListSetName(pParse, yygotominor.yy442, &yymsp[-2].minor.yy0, 1);
        }
            break;
        case 171: /* setlist ::= nm EQ expr */
        {
            yygotominor.yy442 = sqlite3ExprListAppend(pParse, 0, yymsp[0].minor.yy342.pExpr);
            sqlite3ExprListSetName(pParse, yygotominor.yy442, &yymsp[-2].minor.yy0, 1);
        }
            break;
        case 172: /* cmd ::= insert_cmd INTO fullname inscollist_opt valuelist */
        {sqlite3Insert(pParse, yymsp[-2].minor.yy347, yymsp[0].minor.yy487.pList, yymsp[0].minor.yy487.pSelect, yymsp[-1].minor.yy180, yymsp[-4].minor.yy258);}
            break;
        case 173: /* cmd ::= insert_cmd INTO fullname inscollist_opt select */
        {sqlite3Insert(pParse, yymsp[-2].minor.yy347, 0, yymsp[0].minor.yy159, yymsp[-1].minor.yy180, yymsp[-4].minor.yy258);}
            break;
        case 174: /* cmd ::= insert_cmd INTO fullname inscollist_opt DEFAULT VALUES */
        {sqlite3Insert(pParse, yymsp[-3].minor.yy347, 0, 0, yymsp[-2].minor.yy180, yymsp[-5].minor.yy258);}
            break;
        case 175: /* insert_cmd ::= INSERT orconf */
        {yygotominor.yy258 = yymsp[0].minor.yy258;}
            break;
        case 176: /* insert_cmd ::= REPLACE */
        {yygotominor.yy258 = OE_Replace;}
            break;
        case 177: /* valuelist ::= VALUES LP nexprlist RP */
        {
            yygotominor.yy487.pList = yymsp[-1].minor.yy442;
            yygotominor.yy487.pSelect = 0;
        }
            break;
        case 178: /* valuelist ::= valuelist COMMA LP exprlist RP */
        {
            Select *pRight = sqlite3SelectNew(pParse, yymsp[-1].minor.yy442, 0, 0, 0, 0, 0, 0, 0, 0);
            if( yymsp[-4].minor.yy487.pList ){
                yymsp[-4].minor.yy487.pSelect = sqlite3SelectNew(pParse, yymsp[-4].minor.yy487.pList, 0, 0, 0, 0, 0, 0, 0, 0);
                yymsp[-4].minor.yy487.pList = 0;
            }
            yygotominor.yy487.pList = 0;
            if( yymsp[-4].minor.yy487.pSelect==0 || pRight==0 ){
                sqlite3SelectDelete(pParse->db, pRight);
                sqlite3SelectDelete(pParse->db, yymsp[-4].minor.yy487.pSelect);
                yygotominor.yy487.pSelect = 0;
            }else{
                pRight->op = TK_ALL;
                pRight->pPrior = yymsp[-4].minor.yy487.pSelect;
                pRight->selFlags |= SF_Values;
                pRight->pPrior->selFlags |= SF_Values;
                yygotominor.yy487.pSelect = pRight;
            }
        }
            break;
        case 181: /* inscollist ::= inscollist COMMA nm */
        {yygotominor.yy180 = sqlite3IdListAppend(pParse->db,yymsp[-2].minor.yy180,&yymsp[0].minor.yy0);}
            break;
        case 182: /* inscollist ::= nm */
        {yygotominor.yy180 = sqlite3IdListAppend(pParse->db,0,&yymsp[0].minor.yy0);}
            break;
        case 183: /* expr ::= term */
        {yygotominor.yy342 = yymsp[0].minor.yy342;}
            break;
        case 184: /* expr ::= LP expr RP */
        {yygotominor.yy342.pExpr = yymsp[-1].minor.yy342.pExpr; spanSet(&yygotominor.yy342,&yymsp[-2].minor.yy0,&yymsp[0].minor.yy0);}
            break;
        case 185: /* term ::= NULL */
        case 190: /* term ::= INTEGER|FLOAT|BLOB */ yytestcase(yyruleno==190);
        case 191: /* term ::= STRING */ yytestcase(yyruleno==191);
        {spanExpr(&yygotominor.yy342, pParse, yymsp[0].major, &yymsp[0].minor.yy0);}
            break;
        case 186: /* expr ::= id */
        case 187: /* expr ::= JOIN_KW */ yytestcase(yyruleno==187);
        {spanExpr(&yygotominor.yy342, pParse, TK_ID, &yymsp[0].minor.yy0);}
            break;
        case 188: /* expr ::= nm DOT nm */
        {
            Expr *temp1 = sqlite3PExpr(pParse, TK_ID, 0, 0, &yymsp[-2].minor.yy0);
            Expr *temp2 = sqlite3PExpr(pParse, TK_ID, 0, 0, &yymsp[0].minor.yy0);
            yygotominor.yy342.pExpr = sqlite3PExpr(pParse, TK_DOT, temp1, temp2, 0);
            spanSet(&yygotominor.yy342,&yymsp[-2].minor.yy0,&yymsp[0].minor.yy0);
        }
            break;
        case 189: /* expr ::= nm DOT nm DOT nm */
        {
            Expr *temp1 = sqlite3PExpr(pParse, TK_ID, 0, 0, &yymsp[-4].minor.yy0);
            Expr *temp2 = sqlite3PExpr(pParse, TK_ID, 0, 0, &yymsp[-2].minor.yy0);
            Expr *temp3 = sqlite3PExpr(pParse, TK_ID, 0, 0, &yymsp[0].minor.yy0);
            Expr *temp4 = sqlite3PExpr(pParse, TK_DOT, temp2, temp3, 0);
            yygotominor.yy342.pExpr = sqlite3PExpr(pParse, TK_DOT, temp1, temp4, 0);
            spanSet(&yygotominor.yy342,&yymsp[-4].minor.yy0,&yymsp[0].minor.yy0);
        }
            break;
        case 192: /* expr ::= REGISTER */
        {
            /* When doing a nested parse, one can include terms in an expression
             ** that look like this:   #1 #2 ...  These terms refer to registers
             ** in the virtual machine.  #N is the N-th register. */
            if( pParse->nested==0 ){
                sqlite3ErrorMsg(pParse, "near \"%T\": syntax error", &yymsp[0].minor.yy0);
                yygotominor.yy342.pExpr = 0;
            }else{
                yygotominor.yy342.pExpr = sqlite3PExpr(pParse, TK_REGISTER, 0, 0, &yymsp[0].minor.yy0);
                if( yygotominor.yy342.pExpr ) sqlite3GetInt32(&yymsp[0].minor.yy0.z[1], &yygotominor.yy342.pExpr->iTable);
            }
            spanSet(&yygotominor.yy342, &yymsp[0].minor.yy0, &yymsp[0].minor.yy0);
        }
            break;
        case 193: /* expr ::= VARIABLE */
        {
            spanExpr(&yygotominor.yy342, pParse, TK_VARIABLE, &yymsp[0].minor.yy0);
            sqlite3ExprAssignVarNumber(pParse, yygotominor.yy342.pExpr);
            spanSet(&yygotominor.yy342, &yymsp[0].minor.yy0, &yymsp[0].minor.yy0);
        }
            break;
        case 194: /* expr ::= expr COLLATE ids */
        {
            yygotominor.yy342.pExpr = sqlite3ExprAddCollateToken(pParse, yymsp[-2].minor.yy342.pExpr, &yymsp[0].minor.yy0);
            yygotominor.yy342.zStart = yymsp[-2].minor.yy342.zStart;
            yygotominor.yy342.zEnd = &yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n];
        }
            break;
        case 195: /* expr ::= CAST LP expr AS typetoken RP */
        {
            yygotominor.yy342.pExpr = sqlite3PExpr(pParse, TK_CAST, yymsp[-3].minor.yy342.pExpr, 0, &yymsp[-1].minor.yy0);
            spanSet(&yygotominor.yy342,&yymsp[-5].minor.yy0,&yymsp[0].minor.yy0);
        }
            break;
        case 196: /* expr ::= ID LP distinct exprlist RP */
        {
            if( yymsp[-1].minor.yy442 && yymsp[-1].minor.yy442->nExpr>pParse->db->aLimit[SQLITE_LIMIT_FUNCTION_ARG] ){
                sqlite3ErrorMsg(pParse, "too many arguments on function %T", &yymsp[-4].minor.yy0);
            }
            yygotominor.yy342.pExpr = sqlite3ExprFunction(pParse, yymsp[-1].minor.yy442, &yymsp[-4].minor.yy0);
            spanSet(&yygotominor.yy342,&yymsp[-4].minor.yy0,&yymsp[0].minor.yy0);
            if( yymsp[-2].minor.yy305 && yygotominor.yy342.pExpr ){
                yygotominor.yy342.pExpr->flags |= EP_Distinct;
            }
        }
            break;
        case 197: /* expr ::= ID LP STAR RP */
        {
            yygotominor.yy342.pExpr = sqlite3ExprFunction(pParse, 0, &yymsp[-3].minor.yy0);
            spanSet(&yygotominor.yy342,&yymsp[-3].minor.yy0,&yymsp[0].minor.yy0);
        }
            break;
        case 198: /* term ::= CTIME_KW */
        {
            /* The CURRENT_TIME, CURRENT_DATE, and CURRENT_TIMESTAMP values are
             ** treated as functions that return constants */
            yygotominor.yy342.pExpr = sqlite3ExprFunction(pParse, 0,&yymsp[0].minor.yy0);
            if( yygotominor.yy342.pExpr ){
                yygotominor.yy342.pExpr->op = TK_CONST_FUNC;
            }
            spanSet(&yygotominor.yy342, &yymsp[0].minor.yy0, &yymsp[0].minor.yy0);
        }
            break;
        case 199: /* expr ::= expr AND expr */
        case 200: /* expr ::= expr OR expr */ yytestcase(yyruleno==200);
        case 201: /* expr ::= expr LT|GT|GE|LE expr */ yytestcase(yyruleno==201);
        case 202: /* expr ::= expr EQ|NE expr */ yytestcase(yyruleno==202);
        case 203: /* expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr */ yytestcase(yyruleno==203);
        case 204: /* expr ::= expr PLUS|MINUS expr */ yytestcase(yyruleno==204);
        case 205: /* expr ::= expr STAR|SLASH|REM expr */ yytestcase(yyruleno==205);
        case 206: /* expr ::= expr CONCAT expr */ yytestcase(yyruleno==206);
        {spanBinaryExpr(&yygotominor.yy342,pParse,yymsp[-1].major,&yymsp[-2].minor.yy342,&yymsp[0].minor.yy342);}
            break;
        case 207: /* likeop ::= LIKE_KW */
        case 209: /* likeop ::= MATCH */ yytestcase(yyruleno==209);
        {yygotominor.yy318.eOperator = yymsp[0].minor.yy0; yygotominor.yy318.bNot = 0;}
            break;
        case 208: /* likeop ::= NOT LIKE_KW */
        case 210: /* likeop ::= NOT MATCH */ yytestcase(yyruleno==210);
        {yygotominor.yy318.eOperator = yymsp[0].minor.yy0; yygotominor.yy318.bNot = 1;}
            break;
        case 211: /* expr ::= expr likeop expr */
        {
            ExprList *pList;
            pList = sqlite3ExprListAppend(pParse,0, yymsp[0].minor.yy342.pExpr);
            pList = sqlite3ExprListAppend(pParse,pList, yymsp[-2].minor.yy342.pExpr);
            yygotominor.yy342.pExpr = sqlite3ExprFunction(pParse, pList, &yymsp[-1].minor.yy318.eOperator);
            if( yymsp[-1].minor.yy318.bNot ) yygotominor.yy342.pExpr = sqlite3PExpr(pParse, TK_NOT, yygotominor.yy342.pExpr, 0, 0);
            yygotominor.yy342.zStart = yymsp[-2].minor.yy342.zStart;
            yygotominor.yy342.zEnd = yymsp[0].minor.yy342.zEnd;
            if( yygotominor.yy342.pExpr ) yygotominor.yy342.pExpr->flags |= EP_InfixFunc;
        }
            break;
        case 212: /* expr ::= expr likeop expr ESCAPE expr */
        {
            ExprList *pList;
            pList = sqlite3ExprListAppend(pParse,0, yymsp[-2].minor.yy342.pExpr);
            pList = sqlite3ExprListAppend(pParse,pList, yymsp[-4].minor.yy342.pExpr);
            pList = sqlite3ExprListAppend(pParse,pList, yymsp[0].minor.yy342.pExpr);
            yygotominor.yy342.pExpr = sqlite3ExprFunction(pParse, pList, &yymsp[-3].minor.yy318.eOperator);
            if( yymsp[-3].minor.yy318.bNot ) yygotominor.yy342.pExpr = sqlite3PExpr(pParse, TK_NOT, yygotominor.yy342.pExpr, 0, 0);
            yygotominor.yy342.zStart = yymsp[-4].minor.yy342.zStart;
            yygotominor.yy342.zEnd = yymsp[0].minor.yy342.zEnd;
            if( yygotominor.yy342.pExpr ) yygotominor.yy342.pExpr->flags |= EP_InfixFunc;
        }
            break;
        case 213: /* expr ::= expr ISNULL|NOTNULL */
        {spanUnaryPostfix(&yygotominor.yy342,pParse,yymsp[0].major,&yymsp[-1].minor.yy342,&yymsp[0].minor.yy0);}
            break;
        case 214: /* expr ::= expr NOT NULL */
        {spanUnaryPostfix(&yygotominor.yy342,pParse,TK_NOTNULL,&yymsp[-2].minor.yy342,&yymsp[0].minor.yy0);}
            break;
        case 215: /* expr ::= expr IS expr */
        {
            spanBinaryExpr(&yygotominor.yy342,pParse,TK_IS,&yymsp[-2].minor.yy342,&yymsp[0].minor.yy342);
            binaryToUnaryIfNull(pParse, yymsp[0].minor.yy342.pExpr, yygotominor.yy342.pExpr, TK_ISNULL);
        }
            break;
        case 216: /* expr ::= expr IS NOT expr */
        {
            spanBinaryExpr(&yygotominor.yy342,pParse,TK_ISNOT,&yymsp[-3].minor.yy342,&yymsp[0].minor.yy342);
            binaryToUnaryIfNull(pParse, yymsp[0].minor.yy342.pExpr, yygotominor.yy342.pExpr, TK_NOTNULL);
        }
            break;
        case 217: /* expr ::= NOT expr */
        case 218: /* expr ::= BITNOT expr */ yytestcase(yyruleno==218);
        {spanUnaryPrefix(&yygotominor.yy342,pParse,yymsp[-1].major,&yymsp[0].minor.yy342,&yymsp[-1].minor.yy0);}
            break;
        case 219: /* expr ::= MINUS expr */
        {spanUnaryPrefix(&yygotominor.yy342,pParse,TK_UMINUS,&yymsp[0].minor.yy342,&yymsp[-1].minor.yy0);}
            break;
        case 220: /* expr ::= PLUS expr */
        {spanUnaryPrefix(&yygotominor.yy342,pParse,TK_UPLUS,&yymsp[0].minor.yy342,&yymsp[-1].minor.yy0);}
            break;
        case 223: /* expr ::= expr between_op expr AND expr */
        {
            ExprList *pList = sqlite3ExprListAppend(pParse,0, yymsp[-2].minor.yy342.pExpr);
            pList = sqlite3ExprListAppend(pParse,pList, yymsp[0].minor.yy342.pExpr);
            yygotominor.yy342.pExpr = sqlite3PExpr(pParse, TK_BETWEEN, yymsp[-4].minor.yy342.pExpr, 0, 0);
            if( yygotominor.yy342.pExpr ){
                yygotominor.yy342.pExpr->x.pList = pList;
            }else{
                sqlite3ExprListDelete(pParse->db, pList);
            }
            if( yymsp[-3].minor.yy392 ) yygotominor.yy342.pExpr = sqlite3PExpr(pParse, TK_NOT, yygotominor.yy342.pExpr, 0, 0);
            yygotominor.yy342.zStart = yymsp[-4].minor.yy342.zStart;
            yygotominor.yy342.zEnd = yymsp[0].minor.yy342.zEnd;
        }
            break;
        case 226: /* expr ::= expr in_op LP exprlist RP */
        {
            if( yymsp[-1].minor.yy442==0 ){
                /* Expressions of the form
                 **
                 **      expr1 IN ()
                 **      expr1 NOT IN ()
                 **
                 ** simplify to constants 0 (false) and 1 (true), respectively,
                 ** regardless of the value of expr1.
                 */
                yygotominor.yy342.pExpr = sqlite3PExpr(pParse, TK_INTEGER, 0, 0, &sqlite3IntTokens[yymsp[-3].minor.yy392]);
                sqlite3ExprDelete(pParse->db, yymsp[-4].minor.yy342.pExpr);
            }else{
                yygotominor.yy342.pExpr = sqlite3PExpr(pParse, TK_IN, yymsp[-4].minor.yy342.pExpr, 0, 0);
                if( yygotominor.yy342.pExpr ){
                    yygotominor.yy342.pExpr->x.pList = yymsp[-1].minor.yy442;
                    sqlite3ExprSetHeight(pParse, yygotominor.yy342.pExpr);
                }else{
                    sqlite3ExprListDelete(pParse->db, yymsp[-1].minor.yy442);
                }
                if( yymsp[-3].minor.yy392 ) yygotominor.yy342.pExpr = sqlite3PExpr(pParse, TK_NOT, yygotominor.yy342.pExpr, 0, 0);
            }
            yygotominor.yy342.zStart = yymsp[-4].minor.yy342.zStart;
            yygotominor.yy342.zEnd = &yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n];
        }
            break;
        case 227: /* expr ::= LP select RP */
        {
            yygotominor.yy342.pExpr = sqlite3PExpr(pParse, TK_SELECT, 0, 0, 0);
            if( yygotominor.yy342.pExpr ){
                yygotominor.yy342.pExpr->x.pSelect = yymsp[-1].minor.yy159;
                ExprSetProperty(yygotominor.yy342.pExpr, EP_xIsSelect);
                sqlite3ExprSetHeight(pParse, yygotominor.yy342.pExpr);
            }else{
                sqlite3SelectDelete(pParse->db, yymsp[-1].minor.yy159);
            }
            yygotominor.yy342.zStart = yymsp[-2].minor.yy0.z;
            yygotominor.yy342.zEnd = &yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n];
        }
            break;
        case 228: /* expr ::= expr in_op LP select RP */
        {
            yygotominor.yy342.pExpr = sqlite3PExpr(pParse, TK_IN, yymsp[-4].minor.yy342.pExpr, 0, 0);
            if( yygotominor.yy342.pExpr ){
                yygotominor.yy342.pExpr->x.pSelect = yymsp[-1].minor.yy159;
                ExprSetProperty(yygotominor.yy342.pExpr, EP_xIsSelect);
                sqlite3ExprSetHeight(pParse, yygotominor.yy342.pExpr);
            }else{
                sqlite3SelectDelete(pParse->db, yymsp[-1].minor.yy159);
            }
            if( yymsp[-3].minor.yy392 ) yygotominor.yy342.pExpr = sqlite3PExpr(pParse, TK_NOT, yygotominor.yy342.pExpr, 0, 0);
            yygotominor.yy342.zStart = yymsp[-4].minor.yy342.zStart;
            yygotominor.yy342.zEnd = &yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n];
        }
            break;
        case 229: /* expr ::= expr in_op nm dbnm */
        {
            SrcList *pSrc = sqlite3SrcListAppend(pParse->db, 0,&yymsp[-1].minor.yy0,&yymsp[0].minor.yy0);
            yygotominor.yy342.pExpr = sqlite3PExpr(pParse, TK_IN, yymsp[-3].minor.yy342.pExpr, 0, 0);
            if( yygotominor.yy342.pExpr ){
                yygotominor.yy342.pExpr->x.pSelect = sqlite3SelectNew(pParse, 0,pSrc,0,0,0,0,0,0,0);
                ExprSetProperty(yygotominor.yy342.pExpr, EP_xIsSelect);
                sqlite3ExprSetHeight(pParse, yygotominor.yy342.pExpr);
            }else{
                sqlite3SrcListDelete(pParse->db, pSrc);
            }
            if( yymsp[-2].minor.yy392 ) yygotominor.yy342.pExpr = sqlite3PExpr(pParse, TK_NOT, yygotominor.yy342.pExpr, 0, 0);
            yygotominor.yy342.zStart = yymsp[-3].minor.yy342.zStart;
            yygotominor.yy342.zEnd = yymsp[0].minor.yy0.z ? &yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n] : &yymsp[-1].minor.yy0.z[yymsp[-1].minor.yy0.n];
        }
            break;
        case 230: /* expr ::= EXISTS LP select RP */
        {
            Expr *p = yygotominor.yy342.pExpr = sqlite3PExpr(pParse, TK_EXISTS, 0, 0, 0);
            if( p ){
                p->x.pSelect = yymsp[-1].minor.yy159;
                ExprSetProperty(p, EP_xIsSelect);
                sqlite3ExprSetHeight(pParse, p);
            }else{
                sqlite3SelectDelete(pParse->db, yymsp[-1].minor.yy159);
            }
            yygotominor.yy342.zStart = yymsp[-3].minor.yy0.z;
            yygotominor.yy342.zEnd = &yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n];
        }
            break;
        case 231: /* expr ::= CASE case_operand case_exprlist case_else END */
        {
            yygotominor.yy342.pExpr = sqlite3PExpr(pParse, TK_CASE, yymsp[-3].minor.yy122, 0, 0);
            if( yygotominor.yy342.pExpr ){
                yygotominor.yy342.pExpr->x.pList = yymsp[-1].minor.yy122 ? sqlite3ExprListAppend(pParse,yymsp[-2].minor.yy442,yymsp[-1].minor.yy122) : yymsp[-2].minor.yy442;
                sqlite3ExprSetHeight(pParse, yygotominor.yy342.pExpr);
            }else{
                sqlite3ExprListDelete(pParse->db, yymsp[-2].minor.yy442);
                sqlite3ExprDelete(pParse->db, yymsp[-1].minor.yy122);
            }
            yygotominor.yy342.zStart = yymsp[-4].minor.yy0.z;
            yygotominor.yy342.zEnd = &yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n];
        }
            break;
        case 232: /* case_exprlist ::= case_exprlist WHEN expr THEN expr */
        {
            yygotominor.yy442 = sqlite3ExprListAppend(pParse,yymsp[-4].minor.yy442, yymsp[-2].minor.yy342.pExpr);
            yygotominor.yy442 = sqlite3ExprListAppend(pParse,yygotominor.yy442, yymsp[0].minor.yy342.pExpr);
        }
            break;
        case 233: /* case_exprlist ::= WHEN expr THEN expr */
        {
            yygotominor.yy442 = sqlite3ExprListAppend(pParse,0, yymsp[-2].minor.yy342.pExpr);
            yygotominor.yy442 = sqlite3ExprListAppend(pParse,yygotominor.yy442, yymsp[0].minor.yy342.pExpr);
        }
            break;
        case 240: /* nexprlist ::= nexprlist COMMA expr */
        {yygotominor.yy442 = sqlite3ExprListAppend(pParse,yymsp[-2].minor.yy442,yymsp[0].minor.yy342.pExpr);}
            break;
        case 241: /* nexprlist ::= expr */
        {yygotominor.yy442 = sqlite3ExprListAppend(pParse,0,yymsp[0].minor.yy342.pExpr);}
            break;
        case 242: /* cmd ::= createkw uniqueflag INDEX ifnotexists nm dbnm ON nm LP idxlist RP where_opt */
        {
            sqlite3CreateIndex(pParse, &yymsp[-7].minor.yy0, &yymsp[-6].minor.yy0,
                               sqlite3SrcListAppend(pParse->db,0,&yymsp[-4].minor.yy0,0), yymsp[-2].minor.yy442, yymsp[-10].minor.yy392,
                               &yymsp[-11].minor.yy0, yymsp[0].minor.yy122, SQLITE_SO_ASC, yymsp[-8].minor.yy392);
        }
            break;
        case 243: /* uniqueflag ::= UNIQUE */
        case 296: /* raisetype ::= ABORT */ yytestcase(yyruleno==296);
        {yygotominor.yy392 = OE_Abort;}
            break;
        case 244: /* uniqueflag ::= */
        {yygotominor.yy392 = OE_None;}
            break;
        case 247: /* idxlist ::= idxlist COMMA nm collate sortorder */
        {
            Expr *p = sqlite3ExprAddCollateToken(pParse, 0, &yymsp[-1].minor.yy0);
            yygotominor.yy442 = sqlite3ExprListAppend(pParse,yymsp[-4].minor.yy442, p);
            sqlite3ExprListSetName(pParse,yygotominor.yy442,&yymsp[-2].minor.yy0,1);
            sqlite3ExprListCheckLength(pParse, yygotominor.yy442, "index");
            if( yygotominor.yy442 ) yygotominor.yy442->a[yygotominor.yy442->nExpr-1].sortOrder = (u8)yymsp[0].minor.yy392;
        }
            break;
        case 248: /* idxlist ::= nm collate sortorder */
        {
            Expr *p = sqlite3ExprAddCollateToken(pParse, 0, &yymsp[-1].minor.yy0);
            yygotominor.yy442 = sqlite3ExprListAppend(pParse,0, p);
            sqlite3ExprListSetName(pParse, yygotominor.yy442, &yymsp[-2].minor.yy0, 1);
            sqlite3ExprListCheckLength(pParse, yygotominor.yy442, "index");
            if( yygotominor.yy442 ) yygotominor.yy442->a[yygotominor.yy442->nExpr-1].sortOrder = (u8)yymsp[0].minor.yy392;
        }
            break;
        case 249: /* collate ::= */
        {yygotominor.yy0.z = 0; yygotominor.yy0.n = 0;}
            break;
        case 251: /* cmd ::= DROP INDEX ifexists fullname */
        {sqlite3DropIndex(pParse, yymsp[0].minor.yy347, yymsp[-1].minor.yy392);}
            break;
        case 252: /* cmd ::= VACUUM */
        case 253: /* cmd ::= VACUUM nm */ yytestcase(yyruleno==253);
        {sqlite3Vacuum(pParse);}
            break;
        case 254: /* cmd ::= PRAGMA nm dbnm */
        {sqlite3Pragma(pParse,&yymsp[-1].minor.yy0,&yymsp[0].minor.yy0,0,0);}
            break;
        case 255: /* cmd ::= PRAGMA nm dbnm EQ nmnum */
        {sqlite3Pragma(pParse,&yymsp[-3].minor.yy0,&yymsp[-2].minor.yy0,&yymsp[0].minor.yy0,0);}
            break;
        case 256: /* cmd ::= PRAGMA nm dbnm LP nmnum RP */
        {sqlite3Pragma(pParse,&yymsp[-4].minor.yy0,&yymsp[-3].minor.yy0,&yymsp[-1].minor.yy0,0);}
            break;
        case 257: /* cmd ::= PRAGMA nm dbnm EQ minus_num */
        {sqlite3Pragma(pParse,&yymsp[-3].minor.yy0,&yymsp[-2].minor.yy0,&yymsp[0].minor.yy0,1);}
            break;
        case 258: /* cmd ::= PRAGMA nm dbnm LP minus_num RP */
        {sqlite3Pragma(pParse,&yymsp[-4].minor.yy0,&yymsp[-3].minor.yy0,&yymsp[-1].minor.yy0,1);}
            break;
        case 268: /* cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END */
        {
            Token all;
            all.z = yymsp[-3].minor.yy0.z;
            all.n = (int)(yymsp[0].minor.yy0.z - yymsp[-3].minor.yy0.z) + yymsp[0].minor.yy0.n;
            sqlite3FinishTrigger(pParse, yymsp[-1].minor.yy327, &all);
        }
            break;
        case 269: /* trigger_decl ::= temp TRIGGER ifnotexists nm dbnm trigger_time trigger_event ON fullname foreach_clause when_clause */
        {
            sqlite3BeginTrigger(pParse, &yymsp[-7].minor.yy0, &yymsp[-6].minor.yy0, yymsp[-5].minor.yy392, yymsp[-4].minor.yy410.a, yymsp[-4].minor.yy410.b, yymsp[-2].minor.yy347, yymsp[0].minor.yy122, yymsp[-10].minor.yy392, yymsp[-8].minor.yy392);
            yygotominor.yy0 = (yymsp[-6].minor.yy0.n==0?yymsp[-7].minor.yy0:yymsp[-6].minor.yy0);
        }
            break;
        case 270: /* trigger_time ::= BEFORE */
        case 273: /* trigger_time ::= */ yytestcase(yyruleno==273);
        { yygotominor.yy392 = TK_BEFORE; }
            break;
        case 271: /* trigger_time ::= AFTER */
        { yygotominor.yy392 = TK_AFTER;  }
            break;
        case 272: /* trigger_time ::= INSTEAD OF */
        { yygotominor.yy392 = TK_INSTEAD;}
            break;
        case 274: /* trigger_event ::= DELETE|INSERT */
        case 275: /* trigger_event ::= UPDATE */ yytestcase(yyruleno==275);
        {yygotominor.yy410.a = yymsp[0].major; yygotominor.yy410.b = 0;}
            break;
        case 276: /* trigger_event ::= UPDATE OF inscollist */
        {yygotominor.yy410.a = TK_UPDATE; yygotominor.yy410.b = yymsp[0].minor.yy180;}
            break;
        case 279: /* when_clause ::= */
        case 301: /* key_opt ::= */ yytestcase(yyruleno==301);
        { yygotominor.yy122 = 0; }
            break;
        case 280: /* when_clause ::= WHEN expr */
        case 302: /* key_opt ::= KEY expr */ yytestcase(yyruleno==302);
        { yygotominor.yy122 = yymsp[0].minor.yy342.pExpr; }
            break;
        case 281: /* trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI */
        {
            assert( yymsp[-2].minor.yy327!=0 );
            yymsp[-2].minor.yy327->pLast->pNext = yymsp[-1].minor.yy327;
            yymsp[-2].minor.yy327->pLast = yymsp[-1].minor.yy327;
            yygotominor.yy327 = yymsp[-2].minor.yy327;
        }
            break;
        case 282: /* trigger_cmd_list ::= trigger_cmd SEMI */
        {
            assert( yymsp[-1].minor.yy327!=0 );
            yymsp[-1].minor.yy327->pLast = yymsp[-1].minor.yy327;
            yygotominor.yy327 = yymsp[-1].minor.yy327;
        }
            break;
        case 284: /* trnm ::= nm DOT nm */
        {
            yygotominor.yy0 = yymsp[0].minor.yy0;
            sqlite3ErrorMsg(pParse,
                            "qualified table names are not allowed on INSERT, UPDATE, and DELETE "
                            "statements within triggers");
        }
            break;
        case 286: /* tridxby ::= INDEXED BY nm */
        {
            sqlite3ErrorMsg(pParse,
                            "the INDEXED BY clause is not allowed on UPDATE or DELETE statements "
                            "within triggers");
        }
            break;
        case 287: /* tridxby ::= NOT INDEXED */
        {
            sqlite3ErrorMsg(pParse,
                            "the NOT INDEXED clause is not allowed on UPDATE or DELETE statements "
                            "within triggers");
        }
            break;
        case 288: /* trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist where_opt */
        { yygotominor.yy327 = sqlite3TriggerUpdateStep(pParse->db, &yymsp[-4].minor.yy0, yymsp[-1].minor.yy442, yymsp[0].minor.yy122, yymsp[-5].minor.yy258); }
            break;
        case 289: /* trigger_cmd ::= insert_cmd INTO trnm inscollist_opt valuelist */
        {yygotominor.yy327 = sqlite3TriggerInsertStep(pParse->db, &yymsp[-2].minor.yy0, yymsp[-1].minor.yy180, yymsp[0].minor.yy487.pList, yymsp[0].minor.yy487.pSelect, yymsp[-4].minor.yy258);}
            break;
        case 290: /* trigger_cmd ::= insert_cmd INTO trnm inscollist_opt select */
        {yygotominor.yy327 = sqlite3TriggerInsertStep(pParse->db, &yymsp[-2].minor.yy0, yymsp[-1].minor.yy180, 0, yymsp[0].minor.yy159, yymsp[-4].minor.yy258);}
            break;
        case 291: /* trigger_cmd ::= DELETE FROM trnm tridxby where_opt */
        {yygotominor.yy327 = sqlite3TriggerDeleteStep(pParse->db, &yymsp[-2].minor.yy0, yymsp[0].minor.yy122);}
            break;
        case 292: /* trigger_cmd ::= select */
        {yygotominor.yy327 = sqlite3TriggerSelectStep(pParse->db, yymsp[0].minor.yy159); }
            break;
        case 293: /* expr ::= RAISE LP IGNORE RP */
        {
            yygotominor.yy342.pExpr = sqlite3PExpr(pParse, TK_RAISE, 0, 0, 0);
            if( yygotominor.yy342.pExpr ){
                yygotominor.yy342.pExpr->affinity = OE_Ignore;
            }
            yygotominor.yy342.zStart = yymsp[-3].minor.yy0.z;
            yygotominor.yy342.zEnd = &yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n];
        }
            break;
        case 294: /* expr ::= RAISE LP raisetype COMMA nm RP */
        {
            yygotominor.yy342.pExpr = sqlite3PExpr(pParse, TK_RAISE, 0, 0, &yymsp[-1].minor.yy0);
            if( yygotominor.yy342.pExpr ) {
                yygotominor.yy342.pExpr->affinity = (char)yymsp[-3].minor.yy392;
            }
            yygotominor.yy342.zStart = yymsp[-5].minor.yy0.z;
            yygotominor.yy342.zEnd = &yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n];
        }
            break;
        case 295: /* raisetype ::= ROLLBACK */
        {yygotominor.yy392 = OE_Rollback;}
            break;
        case 297: /* raisetype ::= FAIL */
        {yygotominor.yy392 = OE_Fail;}
            break;
        case 298: /* cmd ::= DROP TRIGGER ifexists fullname */
        {
            sqlite3DropTrigger(pParse,yymsp[0].minor.yy347,yymsp[-1].minor.yy392);
        }
            break;
        case 299: /* cmd ::= ATTACH database_kw_opt expr AS expr key_opt */
        {
            sqlite3Attach(pParse, yymsp[-3].minor.yy342.pExpr, yymsp[-1].minor.yy342.pExpr, yymsp[0].minor.yy122);
        }
            break;
        case 300: /* cmd ::= DETACH database_kw_opt expr */
        {
            sqlite3Detach(pParse, yymsp[0].minor.yy342.pExpr);
        }
            break;
        case 305: /* cmd ::= REINDEX */
        {sqlite3Reindex(pParse, 0, 0);}
            break;
        case 306: /* cmd ::= REINDEX nm dbnm */
        {sqlite3Reindex(pParse, &yymsp[-1].minor.yy0, &yymsp[0].minor.yy0);}
            break;
        case 307: /* cmd ::= ANALYZE */
        {sqlite3Analyze(pParse, 0, 0);}
            break;
        case 308: /* cmd ::= ANALYZE nm dbnm */
        {sqlite3Analyze(pParse, &yymsp[-1].minor.yy0, &yymsp[0].minor.yy0);}
            break;
        case 309: /* cmd ::= ALTER TABLE fullname RENAME TO nm */
        {
            sqlite3AlterRenameTable(pParse,yymsp[-3].minor.yy347,&yymsp[0].minor.yy0);
        }
            break;
        case 310: /* cmd ::= ALTER TABLE add_column_fullname ADD kwcolumn_opt column */
        {
            sqlite3AlterFinishAddColumn(pParse, &yymsp[0].minor.yy0);
        }
            break;
        case 311: /* add_column_fullname ::= fullname */
        {
            pParse->db->lookaside.bEnabled = 0;
            sqlite3AlterBeginAddColumn(pParse, yymsp[0].minor.yy347);
        }
            break;
        case 314: /* cmd ::= create_vtab */
        {sqlite3VtabFinishParse(pParse,0);}
            break;
        case 315: /* cmd ::= create_vtab LP vtabarglist RP */
        {sqlite3VtabFinishParse(pParse,&yymsp[0].minor.yy0);}
            break;
        case 316: /* create_vtab ::= createkw VIRTUAL TABLE ifnotexists nm dbnm USING nm */
        {
            sqlite3VtabBeginParse(pParse, &yymsp[-3].minor.yy0, &yymsp[-2].minor.yy0, &yymsp[0].minor.yy0, yymsp[-4].minor.yy392);
        }
            break;
        case 319: /* vtabarg ::= */
        {sqlite3VtabArgInit(pParse);}
            break;
        case 321: /* vtabargtoken ::= ANY */
        case 322: /* vtabargtoken ::= lp anylist RP */ yytestcase(yyruleno==322);
        case 323: /* lp ::= LP */ yytestcase(yyruleno==323);
        {sqlite3VtabArgExtend(pParse,&yymsp[0].minor.yy0);}
            break;
        default:
            /* (0) input ::= cmdlist */ yytestcase(yyruleno==0);
            /* (1) cmdlist ::= cmdlist ecmd */ yytestcase(yyruleno==1);
            /* (2) cmdlist ::= ecmd */ yytestcase(yyruleno==2);
            /* (3) ecmd ::= SEMI */ yytestcase(yyruleno==3);
            /* (4) ecmd ::= explain cmdx SEMI */ yytestcase(yyruleno==4);
            /* (10) trans_opt ::= */ yytestcase(yyruleno==10);
            /* (11) trans_opt ::= TRANSACTION */ yytestcase(yyruleno==11);
            /* (12) trans_opt ::= TRANSACTION nm */ yytestcase(yyruleno==12);
            /* (20) savepoint_opt ::= SAVEPOINT */ yytestcase(yyruleno==20);
            /* (21) savepoint_opt ::= */ yytestcase(yyruleno==21);
            /* (25) cmd ::= create_table create_table_args */ yytestcase(yyruleno==25);
            /* (34) columnlist ::= columnlist COMMA column */ yytestcase(yyruleno==34);
            /* (35) columnlist ::= column */ yytestcase(yyruleno==35);
            /* (44) type ::= */ yytestcase(yyruleno==44);
            /* (51) signed ::= plus_num */ yytestcase(yyruleno==51);
            /* (52) signed ::= minus_num */ yytestcase(yyruleno==52);
            /* (53) carglist ::= carglist ccons */ yytestcase(yyruleno==53);
            /* (54) carglist ::= */ yytestcase(yyruleno==54);
            /* (61) ccons ::= NULL onconf */ yytestcase(yyruleno==61);
            /* (89) conslist ::= conslist tconscomma tcons */ yytestcase(yyruleno==89);
            /* (90) conslist ::= tcons */ yytestcase(yyruleno==90);
            /* (92) tconscomma ::= */ yytestcase(yyruleno==92);
            /* (277) foreach_clause ::= */ yytestcase(yyruleno==277);
            /* (278) foreach_clause ::= FOR EACH ROW */ yytestcase(yyruleno==278);
            /* (285) tridxby ::= */ yytestcase(yyruleno==285);
            /* (303) database_kw_opt ::= DATABASE */ yytestcase(yyruleno==303);
            /* (304) database_kw_opt ::= */ yytestcase(yyruleno==304);
            /* (312) kwcolumn_opt ::= */ yytestcase(yyruleno==312);
            /* (313) kwcolumn_opt ::= COLUMNKW */ yytestcase(yyruleno==313);
            /* (317) vtabarglist ::= vtabarg */ yytestcase(yyruleno==317);
            /* (318) vtabarglist ::= vtabarglist COMMA vtabarg */ yytestcase(yyruleno==318);
            /* (320) vtabarg ::= vtabarg vtabargtoken */ yytestcase(yyruleno==320);
            /* (324) anylist ::= */ yytestcase(yyruleno==324);
            /* (325) anylist ::= anylist LP anylist RP */ yytestcase(yyruleno==325);
            /* (326) anylist ::= anylist ANY */ yytestcase(yyruleno==326);
            break;
    };
    assert( yyruleno>=0 && yyruleno<sizeof(yyRuleInfo)/sizeof(yyRuleInfo[0]) );
    yygoto = yyRuleInfo[yyruleno].lhs;
    yysize = yyRuleInfo[yyruleno].nrhs;
    yypParser->yyidx -= yysize;
    yyact = yy_find_reduce_action(yymsp[-yysize].stateno,(YYCODETYPE)yygoto);
    if( yyact < YYNSTATE ){
#ifdef NDEBUG
        /* If we are not debugging and the reduce action popped at least
         ** one element off the stack, then we can push the new element back
         ** onto the stack here, and skip the stack overflow test in yy_shift().
         ** That gives a significant speed improvement. */
        if( yysize ){
            yypParser->yyidx++;
            yymsp -= yysize-1;
            yymsp->stateno = (YYACTIONTYPE)yyact;
            yymsp->major = (YYCODETYPE)yygoto;
            yymsp->minor = yygotominor;
        }else
#endif
        {
            yy_shift(yypParser,yyact,yygoto,&yygotominor);
        }
    }else{
        assert( yyact == YYNSTATE + YYNRULE + 1 );
        yy_accept(yypParser);
    }
}

/*
 ** The following code executes when the parse fails
 */
#ifndef YYNOERRORRECOVERY
static void yy_parse_failed(
                            yyParser *yypParser           /* The parser */
){
    sqlite3ParserARG_FETCH;
#ifndef NDEBUG
    if( yyTraceFILE ){
        fprintf(yyTraceFILE,"%sFail!\n",yyTracePrompt);
    }
#endif
    while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
    /* Here code is inserted which will be executed whenever the
     ** parser fails */
    sqlite3ParserARG_STORE; /* Suppress warning about unused %extra_argument variable */
}
#endif /* YYNOERRORRECOVERY */

/*
 ** The following code executes when a syntax error first occurs.
 */
static void yy_syntax_error(
                            yyParser *yypParser,           /* The parser */
                            int yymajor,                   /* The major type of the error token */
                            YYMINORTYPE yyminor            /* The minor type of the error token */
){
    sqlite3ParserARG_FETCH;
#define TOKEN (yyminor.yy0)
    
    UNUSED_PARAMETER(yymajor);  /* Silence some compiler warnings */
    assert( TOKEN.z[0] );  /* The tokenizer always gives us a token */
    sqlite3ErrorMsg(pParse, "near \"%T\": syntax error", &TOKEN);
    sqlite3ParserARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/*
 ** The following is executed when the parser accepts
 */
static void yy_accept(
                      yyParser *yypParser           /* The parser */
){
    sqlite3ParserARG_FETCH;
#ifndef NDEBUG
    if( yyTraceFILE ){
        fprintf(yyTraceFILE,"%sAccept!\n",yyTracePrompt);
    }
#endif
    while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
    /* Here code is inserted which will be executed whenever the
     ** parser accepts */
    sqlite3ParserARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/* The main parser program.
 ** The first argument is a pointer to a structure obtained from
 ** "sqlite3ParserAlloc" which describes the current state of the parser.
 ** The second argument is the major token number.  The third is
 ** the minor token.  The fourth optional argument is whatever the
 ** user wants (and specified in the grammar) and is available for
 ** use by the action routines.
 **
 ** Inputs:
 ** <ul>
 ** <li> A pointer to the parser (an opaque structure.)
 ** <li> The major token number.
 ** <li> The minor token number.
 ** <li> An option argument of a grammar-specified type.
 ** </ul>
 **
 ** Outputs:
 ** None.
 */
SQLITE_PRIVATE void sqlite3Parser(
                                  void *yyp,                   /* The parser */
                                  int yymajor,                 /* The major token code number */
                                  sqlite3ParserTOKENTYPE yyminor       /* The value for the token */
                                  sqlite3ParserARG_PDECL               /* Optional %extra_argument parameter */
){
    YYMINORTYPE yyminorunion;
    int yyact;            /* The parser action. */
#if !defined(YYERRORSYMBOL) && !defined(YYNOERRORRECOVERY)
    int yyendofinput;     /* True if we are at the end of input */
#endif
#ifdef YYERRORSYMBOL
    int yyerrorhit = 0;   /* True if yymajor has invoked an error */
#endif
    yyParser *yypParser;  /* The parser */
    
    /* (re)initialize the parser, if necessary */
    yypParser = (yyParser*)yyp;
    if( yypParser->yyidx<0 ){
#if YYSTACKDEPTH<=0
        if( yypParser->yystksz <=0 ){
            /*memset(&yyminorunion, 0, sizeof(yyminorunion));*/
            yyminorunion = yyzerominor;
            yyStackOverflow(yypParser, &yyminorunion);
            return;
        }
#endif
        yypParser->yyidx = 0;
        yypParser->yyerrcnt = -1;
        yypParser->yystack[0].stateno = 0;
        yypParser->yystack[0].major = 0;
    }
    yyminorunion.yy0 = yyminor;
#if !defined(YYERRORSYMBOL) && !defined(YYNOERRORRECOVERY)
    yyendofinput = (yymajor==0);
#endif
    sqlite3ParserARG_STORE;
    
#ifndef NDEBUG
    if( yyTraceFILE ){
        fprintf(yyTraceFILE,"%sInput %s\n",yyTracePrompt,yyTokenName[yymajor]);
    }
#endif
    
    do{
        yyact = yy_find_shift_action(yypParser,(YYCODETYPE)yymajor);
        if( yyact<YYNSTATE ){
            yy_shift(yypParser,yyact,yymajor,&yyminorunion);
            yypParser->yyerrcnt--;
            yymajor = YYNOCODE;
        }else if( yyact < YYNSTATE + YYNRULE ){
            yy_reduce(yypParser,yyact-YYNSTATE);
        }else{
            assert( yyact == YY_ERROR_ACTION );
#ifdef YYERRORSYMBOL
            int yymx;
#endif
#ifndef NDEBUG
            if( yyTraceFILE ){
                fprintf(yyTraceFILE,"%sSyntax Error!\n",yyTracePrompt);
            }
#endif
#ifdef YYERRORSYMBOL
            /* A syntax error has occurred.
             ** The response to an error depends upon whether or not the
             ** grammar defines an error token "ERROR".  
             **
             ** This is what we do if the grammar does define ERROR:
             **
             **  * Call the %syntax_error function.
             **
             **  * Begin popping the stack until we enter a state where
             **    it is legal to shift the error symbol, then shift
             **    the error symbol.
             **
             **  * Set the error count to three.
             **
             **  * Begin accepting and shifting new tokens.  No new error
             **    processing will occur until three tokens have been
             **    shifted successfully.
             **
             */
            if( yypParser->yyerrcnt<0 ){
                yy_syntax_error(yypParser,yymajor,yyminorunion);
            }
            yymx = yypParser->yystack[yypParser->yyidx].major;
            if( yymx==YYERRORSYMBOL || yyerrorhit ){
#ifndef NDEBUG
                if( yyTraceFILE ){
                    fprintf(yyTraceFILE,"%sDiscard input token %s\n",
                            yyTracePrompt,yyTokenName[yymajor]);
                }
#endif
                yy_destructor(yypParser, (YYCODETYPE)yymajor,&yyminorunion);
                yymajor = YYNOCODE;
            }else{
                while(
                      yypParser->yyidx >= 0 &&
                      yymx != YYERRORSYMBOL &&
                      (yyact = yy_find_reduce_action(
                                                     yypParser->yystack[yypParser->yyidx].stateno,
                                                     YYERRORSYMBOL)) >= YYNSTATE
                      ){
                    yy_pop_parser_stack(yypParser);
                }
                if( yypParser->yyidx < 0 || yymajor==0 ){
                    yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
                    yy_parse_failed(yypParser);
                    yymajor = YYNOCODE;
                }else if( yymx!=YYERRORSYMBOL ){
                    YYMINORTYPE u2;
                    u2.YYERRSYMDT = 0;
                    yy_shift(yypParser,yyact,YYERRORSYMBOL,&u2);
                }
            }
            yypParser->yyerrcnt = 3;
            yyerrorhit = 1;
#elif defined(YYNOERRORRECOVERY)
            /* If the YYNOERRORRECOVERY macro is defined, then do not attempt to
             ** do any kind of error recovery.  Instead, simply invoke the syntax
             ** error routine and continue going as if nothing had happened.
             **
             ** Applications can set this macro (for example inside %include) if
             ** they intend to abandon the parse upon the first syntax error seen.
             */
            yy_syntax_error(yypParser,yymajor,yyminorunion);
            yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
            yymajor = YYNOCODE;
            
#else  /* YYERRORSYMBOL is not defined */
            /* This is what we do if the grammar does not define ERROR:
             **
             **  * Report an error message, and throw away the input token.
             **
             **  * If the input token is $, then fail the parse.
             **
             ** As before, subsequent error messages are suppressed until
             ** three input tokens have been successfully shifted.
             */
            if( yypParser->yyerrcnt<=0 ){
                yy_syntax_error(yypParser,yymajor,yyminorunion);
            }
            yypParser->yyerrcnt = 3;
            yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
            if( yyendofinput ){
                yy_parse_failed(yypParser);
            }
            yymajor = YYNOCODE;
#endif
        }
    }while( yymajor!=YYNOCODE && yypParser->yyidx>=0 );
    return;
}

/************** End of parse.c ***********************************************/