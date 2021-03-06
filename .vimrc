" use indentation of previous line
set autoindent
" use intelligent indentation
set smartindent
" configure tabwidth and insert spaces instead of tabs
set tabstop=2        " tab width is 4 spaces
set shiftwidth=2     " indent also with 4 spaces
set textwidth=80     " set the textwidth wrapping to 80 chars
set expandtab        " expand tabs to spaces
set laststatus=2     " status bar bottom

set scrolloff=4 " keep scroll off the page top/bottom
set number relativenumber " line numbers
set ruler " vertical and horizontal location
set incsearch " highlights when searching
set hlsearch " highlight words after searching
set autoread " automatically loads changed files
set cursorline " shows what line your cursor is on
set whichwrap+=<,>,[,],h,l,~                 " arrow keys can wrap in normal and insert modes
set backspace=indent,eol,start
set wildignorecase "when doing autocomplete ignore the case


syntax enable " syntax highlighting      
set background=light
let g:solarized_termcolors=256
let g:solarized_termtrans=1
let g:solarized_bold=1
let g:solarized_italic=1
colorscheme solarized 

set foldmethod=syntax

inoremap ;; <Right>
" basic brace completion
inoremap {      {}<Left>
inoremap {<CR>  {<CR>}<Esc>O
inoremap {}     {}

inoremap (      ()<Left>
inoremap (<CR>  (<CR>)<Esc>O
inoremap ()     ()

inoremap [      []<left>
inoremap [<cr>  [<cr>]<esc>o
inoremap []     []

" quotations
inoremap "      ""<left>
inoremap "<cr>  "<cr>"<esc>o
inoremap ""     ""

inoremap '      ''<left>
inoremap '<cr>  '<cr>'<esc>o
inoremap ''     ''

set formatoptions+=rco

set mouse=a " mouse stuff point n click

inoremap jk <Esc>

" tab autocomplete
inoremap <Tab> <C-r>=SuperTab()<CR>
inoremap <S-Tab> <C-x><C-f>

function! SuperTab()
  let l:part = strpart(getline('.'),col('.')-2,1)
  if (l:part=~'^\W\?$')
      return "\<Tab>"
  else
      return "\<C-n>"
  endif
endfunction

let mapleader = "\<Space>"

" highlight EOL whitespace
highlight ExtraWhitespace ctermbg=lightred guibg=lightred
match ExtraWhitespace /\s\+$/  

" tabs
nnoremap <leader>t :tabedit<CR>:edit<Space>
nnoremap <C-k> :tabnext<CR>
nnoremap <C-j> :tabprev<CR>
nnoremap <leader>n :edit<Space>
nnoremap <leader><leader>n :Explore<CR>

" writing and closing a file
nnoremap <leader>s :w<CR> 
nnoremap <leader>q :q<CR>
nnoremap <leader>x :qa<CR>

" making a vim session
nnoremap <leader>m <Esc>:mksession! session.vim<CR>
" reloading changed to vimrc
nnoremap <leader>r <Esc>:so ~/.vimrc<CR>

" clearing the seach buffer
nnoremap ,, /;;;<CR>

"vim splits
nnoremap <leader>v :vsplit<CR><C-w>l:edit<Space>
nnoremap <leader>b :split<CR><C-w>j:edit<Space>
nnoremap <leader>h <C-w>h
nnoremap <leader>l <C-w>l
nnoremap <leader>j <C-w>j
nnoremap <leader>k <C-w>k

" search and replace
nnoremap <leader>f :%s///gc<Left><Left><Left><Left>

" multiline formatting
vnoremap <leader>f JAa<BS><Esc>

" copy and paste formatting
nnoremap <leader>c :set<Space>mouse=<CR>:set<Space>nonumber<CR>:set<Space>nornu<CR>
nnoremap <leader><leader>c :set<Space>mouse=a<CR>:set<Space>number<CR>:set<Space>relativenumber<CR>
nnoremap <leader>p :set<Space>paste<CR>i
nnoremap <leader><leader>p :set<Space>nopaste<CR>

" Tagging
command! MakeTags !ctags -Rf .tags *
nnoremap <leader>[ :MakeTags<CR><CR>
set tags=tags;/

" load typescript files
autocmd BufNewFile,BufRead *.ts setlocal filetype=typescript

""" FILETYPE SPECIFIC:
" .vim file loader
function! Load_File(file) abort
  if !empty(globpath(&runtimepath, a:file))
    execute "source " . globpath(&runtimepath, a:file)
  endif
endfunction
if matchstr(&runtimepath, $HOME.'/.vim/lang') == ""
  let &runtimepath.=','.$HOME.'/.vim/lang'
endif
augroup au_langs
  autocmd!
  " C/C++
  autocmd filetype c,cpp call Load_File("c.vim")
  " Java
  autocmd filetype java call Load_File("java.vim")
  " LaTeX
  let g:tex_flavor = "latex"
  autocmd filetype tex call Load_File("tex.vim")
  " Python
  autocmd filetype python call Load_File("python.vim")
  " Typescript
  autocmd filetype typescript call Load_File("typescript.vim")
  " Javascript
  autocmd filetype javascript call Load_File("javascript.vim")
augroup END
