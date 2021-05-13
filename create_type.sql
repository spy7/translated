-- DROP TABLE traduz;
-- DROP TYPE i18n.translated CASCADE;

CREATE TYPE i18n.translated;

CREATE FUNCTION i18n.translated_in(cstring)
    RETURNS i18n.translated
    AS 'translated'
    LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION i18n.translated_out(i18n.translated)
    RETURNS cstring
    AS 'translated'
    LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION i18n.add_text(i18n.translated, cstring)
    RETURNS i18n.translated
    AS 'translated', 'add_text'
    LANGUAGE C IMMUTABLE STRICT;
   
CREATE FUNCTION i18n.create_text(cstring)
    RETURNS i18n.translated
    AS 'translated', 'create_text'
    LANGUAGE C IMMUTABLE STRICT;
   
CREATE FUNCTION i18n.compare_text(i18n.translated, i18n.translated)
    RETURNS bool
    AS 'translated', 'compare_text'
    LANGUAGE C IMMUTABLE STRICT;
   
CREATE FUNCTION i18n.compare_text_not(i18n.translated, i18n.translated)
    RETURNS bool
    AS 'translated', 'compare_text_not'
    LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION i18n.compare_string(i18n.translated, cstring)
    RETURNS bool
    AS 'translated', 'compare_string'
    LANGUAGE C IMMUTABLE STRICT;
   
CREATE FUNCTION i18n.compare_string_not(i18n.translated, cstring)
    RETURNS bool
    AS 'translated', 'compare_string_not'
    LANGUAGE C IMMUTABLE STRICT;
   
CREATE TYPE i18n.translated (
   internallength = VARIABLE,
   input = i18n.translated_in,
   output = i18n.translated_out
);

CREATE OPERATOR # (
    leftarg = i18n.translated,
    rightarg = cstring,
    procedure = i18n.add_text,
    commutator = #
);

CREATE OPERATOR ! (
    rightarg = cstring,
    procedure = i18n.create_text
);

CREATE OPERATOR = (
    leftarg = i18n.translated,
    rightarg = i18n.translated,
    procedure = i18n.compare_text
);

CREATE OPERATOR <> (
    leftarg = i18n.translated,
    rightarg = i18n.translated,
    procedure = i18n.compare_text_not
);

CREATE OPERATOR = (
    leftarg = i18n.translated,
    rightarg = cstring,
    procedure = i18n.compare_string
);

CREATE OPERATOR == (
    leftarg = i18n.translated,
    rightarg = cstring,
    procedure = i18n.compare_string
);

CREATE OPERATOR <> (
    leftarg = i18n.translated,
    rightarg = cstring,
    procedure = i18n.compare_string_not
);

CREATE OPERATOR !== (
    leftarg = i18n.translated,
    rightarg = cstring,
    procedure = i18n.compare_string_not
);

-- CREATE TABLE public.traduz (
-- 	id serial not null,
--     valor i18n.translated NULL,
--     nome text
-- );

-- INSERT INTO traduz(nome, valor) VALUES ('oi', '%TRL(2,2,en,1,b)');
-- INSERT INTO traduz(nome, valor) VALUES ('oi', '%TRL(2,2,pt,3,Ops)');
-- INSERT INTO traduz(nome, valor) VALUES ('oi', '%TRL(2,2,pt,4,Opss)');
-- INSERT INTO traduz(nome, valor) VALUES ('oi', '%TRL(4,2,pt,3,Ops,1,o,7,488addf)');
-- INSERT INTO traduz(nome, valor) VALUES ('oi', '%TRL(0)');
-- INSERT INTO traduz(nome, valor) VALUES ('oi', !'e');
-- INSERT INTO traduz(nome, valor) VALUES ('oi', 'f');
-- UPDATE traduz SET valor = valor#'haha' WHERE id = 41;
-- UPDATE traduz SET valor = valor WHERE nome = 'oi';
-- UPDATE traduz SET valor = NULL WHERE id = 176;

-- DELETE FROM traduz WHERE true;

-- SELECT * FROM traduz ORDER BY id DESC;

-- SELECT * FROM traduz WHERE valor <> 'b';


-- SET trl.lang = 'en';
-- SET trl.input = TRUE;
-- SET trl.first = FALSE;
-- SET trl.output = TRUE;
-- SELECT current_setting('trl.lang', true);

