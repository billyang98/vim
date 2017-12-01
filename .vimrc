" use indentation of previous line
set autoindent
" use intelligent indentation for C
set smartindent
" configure tabwidth and insert spaces instead of tabs
set tabstop=4        " tab width is 4 spaces
set shiftwidth=4     " indent also with 4 spaces
set expandtab        " expand tabs to spaces
set laststatus=2     " status bar bottom

set scrolloff=4 " keep scroll off the page top/bottom
set number relativenumber " line numbers
set ruler " vertical and horizontal location
set incsearch " highlights when searching
set hlsearch " highlight words after searching
set autoread " automatically loads changed files
" set cursorline " shows what line your cursor is on 

syntax enable " syntax highlighting
set background=dark
let g:solarized_termcolors=256
let g:solarized_termtrans=1
let g:solarized_bold=1
let g:solarized_italic=1
colorscheme solarized 

" cpp syntax
let g:cpp_class_scope_highlight = 1
let g:cpp_member_variable_highlight = 1
let g:cpp_class_decl_highlight = 1
let g:cpp_class_decl_highlight = 1
let g:cpp_experimental_template_highlight = 1

" basic curly brace completion
inoremap {      {}<Left>
inoremap {<CR>  {<CR>}<Esc>O
inoremap {}     {}

inoremap (      ()<Left>
inoremap (<CR>  (<CR>)<Esc>O
inoremap ()     ()

set formatoptions+=rco

set mouse=a " mouse stuff point n click
set cursorline

inoremap jk <Esc>
vnoremap jk <Esc> 

" tab autocomplete
inoremap <Tab> <C-r>=SuperTab()<CR>

function! SuperTab()
  let l:part = strpart(getline('.'),col('.')-2,1)
  if (l:part=~'^\W\?$')
      return "\<Tab>"
  else
      return "\<C-n>"
  endif
endfunction

let mapleader = "\<Space>"
" tab
nnoremap <leader>t :tabedit<CR>:edit<Space>
nnoremap <leader>k :tabnext<CR>
nnoremap <leader>j :tabprev<CR>
nnoremap <leader>n :edit<Space>

" writing and closing a file
nnoremap <leader>s <Esc>:w<CR> 
nnoremap <leader>q <Esc>:q<CR>
nnoremap <leader>x <Esc>:qa<CR>

" making a vim session
nnoremap <leader>m <Esc>:mksession! session.vim<CR>
" reloading changed to vimrc
nnoremap <leader>r <Esc>:so ~/.vimrc<CR>

" clearing the seach buffer
nnoremap ,, /;;;<CR>

" commenting a line
nnoremap <leader>/ 0i//<Esc>
nnoremap <leader><leader>/ 0xx
" commenting multiple lines in visual block
vnoremap <leader>/ I//<Esc>
vnoremap <leader><leader>/ ld<Esc>

"vim splits
nnoremap <leader>v :vsplit<CR>
nnoremap <leader>b :split<CR>
nnoremap <C-h> <C-w>h
nnoremap <C-l> <C-w>l
nnoremap <C-j> <C-w>j
nnoremap <C-k> <C-w>k

" search and replace
nnoremap <leader>f :%s///gc

nnoremap <C-c> :set<Space>nonumber<CR>:set<Space>nornu<CR>
nnoremap <C-x> :set<Space>number<CR>:set<Space>relativenumber<CR>
nnoremap <leader>p :set<Space>paste<CR>i
nnoremap <leader><leader>p :set<Space>nopaste<CR>


