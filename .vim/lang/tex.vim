inoremap ;bg <Esc>:call<Space>BeginEnd("")<CR>

" commenting a line
nnoremap <leader>/ 0i%<Esc>
nnoremap <leader><leader>/ 0x
" commenting multiple lines in visual block
vnoremap <leader>/ I%<Esc>
vnoremap <leader><leader>/ d<Esc>

" \begin{} ... \end{
function! BeginEnd(type) abort
  let t = a:type
  if !(strlen(a:type))
    call inputsave()
    let t = input('Begin: ')
    call inputrestore()
  endif
  let ins = ["\\begin{".t."}", "\\end{".t."}"]
  call append('.', ins)
  delete
endfunction

" compile
nnoremap <leader>cp :call CompileTex()<CR>

function! CompileTex() abort
  let cmd = "!pdflatex ".expand('%:p')
  execute cmd
endfunction

" more latex shortcuts
inoremap $$ $$<Left>
inoremap ;it \textit{}<Left>
inoremap ;bf \textbf{}<Left>
inoremap ;f/ \frac{}{}<Left><Left><Left>
inoremap \{ \{\}<Left><Left>
