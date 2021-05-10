# translated

## Configurar

* `SET my.lang='pt-bt';`

## Select

* Se configurado, vem apenas o valor da língua escolhida (ou da primeira caso NULL)
* `SELECT campo FROM tabela;`

## Insert

* Usar "!" para enviar mensagem na língua
* `INSERT INTO tabela(campo) VALUES (!'meu texto');`

## Update

* Concatenar o valor anterior com ":"
* `UPDATE tabela SET campo = campo:'meu texto';`
