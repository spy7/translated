# Translated

## Formato

* `%TRL(n,m,xxx,m,xxx,...)`
* n = número de textos
* m = número de caracteres
* x = texto

## Configurar idioma

* `SET trl.lang = 'pt';`

### Select

* Se configurado, vem apenas o valor da língua escolhida (ou da primeira língua caso não tenha)
* `SELECT campo FROM tabela;`

### Insert

* Usar "!" para enviar mensagem na língua
* `INSERT INTO tabela(campo) VALUES (!'meu texto');`

### Update

* Concatenar o valor anterior com "#"
* `UPDATE tabela SET campo = campo#'meu texto';`

### Comparar com texto

* Usar cast, "!" ou o operator "=="
* `SELECT campo FROM tabela WHERE campo = 'meu texto'::cstring;`
* `SELECT campo FROM tabela WHERE campo = !'meu texto';`
* `SELECT campo FROM tabela WHERE campo == 'meu texto';`

## Configurar INPUT automático

* `SET trl.input = TRUE;`

### Insert

* Não precisa mais do "!"
* `INSERT INTO tabela(campo) VALUES ('meu texto');`

### Update

* Continuar usando o "#"
* `UPDATE tabela SET campo = campo#'meu texto';`

* AVISO: enviar um valor direto não dará mais problema, mas irá remover todos os outros idiomas
* `UPDATE tabela SET campo = 'meu texto';`

### Comparar com texto

* Pode fazer diretamente (será feito a conversão do valor como um "!")
* `SELECT campo FROM tabela WHERE campo = 'meu texto'`

## Configurar OUTPUT

* `SET trl.output = FALSE;`
* Irá exibir o formato original no SELECT (não apenas a língua escolhida)

## Configurar FIRST

* `SET trl.first = TRUE;`
* Quando não encontrar a língua selecionada, não irá mais trazer o primeiro idioma.
