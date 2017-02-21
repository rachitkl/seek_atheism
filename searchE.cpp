#include <bits/stdc++.h>
#include <fstream>
#include <omp.h>
#include <dirent.h>
#include <boost/tokenizer.hpp>
#include <string>
#include <iostream>
#include "stemming/english_stem.h"
using namespace std;
bool verbose = 0;
struct trie
{
	map <char,trie*>head;
	vector <pair<int,double > > doc;
	double count;
	trie()
	{
		count = 0.0;
		head.clear();
		doc.clear();
	}
};
vector<pair<string,double> >files;
map<char,trie*>head;

string to_low(string A)
{
	string g = "";
	for(int i=0;i<A.size();i++)
	{
		g += (A[i]<='Z' && A[i]>='A')?A[i]+'a'-'A':A[i];
	}
	return g;
}

//Normalize a given string to some root word
string normalize(string s)
{
    	if(s == "")return "";
    	std::wstring word(L"something");

	wchar_t* UnicodeTextBuffer = new wchar_t[s.length()+1];
    	std::wmemset(UnicodeTextBuffer, 0, s.length()+1);
    	std::mbstowcs(UnicodeTextBuffer, s.c_str(), s.length());

	word = UnicodeTextBuffer;
    	stemming::english_stem<> StemEnglish;
	StemEnglish(word);

	// Stemmed string being stored in "st"
    	std::string st(word.begin(), word.end());
	//cout<<"n"<<endl;
	return st;
}


//tokenizing a given string "s"
vector<string> toke(string &s)
{
	typedef boost::tokenizer<boost::char_separator<char>> tokenizer;
	boost::char_separator<char> sep{" ,?!;.:"};
	tokenizer tok{s, sep};
	// I have taken a vector assuming the string gives 
	// out multiple tokens
	vector<string> tokens;
	for (auto &t : tok)
	{
        	//std::cout << t << '\n';
		string st(normalize(to_low(t)));
		if(st!="")
		tokens.push_back(st);
	}
	//cout<<"t"<<endl;
	return tokens;
}
 


// Take string input. '.' acts as a terminator
vector<string>take_line()
{
	vector<string>Q;
	string g="";
	while(1)
	{
		cin>>g;
		if(g == ".")break;
		if(g[g.size()-1] == '.')
		{
			Q.push_back(g.substr(0,g.size()-1));
			break;	
		}
		else	
		Q.push_back(g);
	}
	return Q;
}
vector<string> chkdir (string dir)
{
    DIR *dp;
    vector<string>files;
    struct dirent *dirp;
    if((dp  = opendir(dir.c_str())) == NULL) 
    {
        return files;
    }
    while ((dirp = readdir(dp)) != NULL) 
    {
        files.push_back(string(dirp->d_name));
    }
    closedir(dp);
    return files;
}
map<string,long long> freader(string file)
{
	map<string,long long>ans;
	ifstream in(file.c_str());
	string s;
	while(in>>s)
	{
		vector<string>t;
		t = toke(s);
		for(int i=0;i<t.size();i++)
		ans[t[i]]++;
	}
	in.close();
	//cout<<file<<" OK"<<endl;
	return ans;
}
string dirchk()
{
	string a;
	cout<<"Enter the name of the directory you would like to pre-process \n";
	cin>>a;
	if(chkdir(a).size() == 0)
	{
		cout<<"Directory "<<a<<" does not exist\nExiting\n";
		return "";	
	}
	cout<<"Directory "<<a<<" found, now Preprocessing\nPlease note that this might take a while"<<endl;
    return a;
}
void insert_trie(string &s,int doc_id,int i,int num,map<char,trie*>&t)
{
	if(t.find(s[i]) == t.end())
	{
		t[s[i]] = new trie();
	}
	if(i==s.size()-1)
	{	
		t[s[i]]->doc.push_back(make_pair(doc_id,num));
		t[s[i]]->count+=num;
	}
	else
	{
		insert_trie(s,doc_id,i+1,num,t[s[i]]->head);
	}
}
vector<pair<int,double> > find_trie(string &s,int i,map<char,trie*>&t,double * count)
{
	if(t.find(s[i]) == t.end())
	{
		vector<pair<int,double> >v;
		return v;
	}
	if(i==s.size()-1)
	{
		(*count) = t[s[i]]->count;
		return t[s[i]]->doc;
	}
	return find_trie(s,i+1,t[s[i]]->head,count);
}
void dreader(string s)
{
	if(verbose)cout<<"Parsing Files in Folder "<<s<<endl;
	vector <string> file;
	file = chkdir(s);
	for(int i=0;i<file.size();i++)
	{
		if(file[i] == "." || file[i] == "..")continue;
		if(chkdir(s+"/"+file[i]).size() == 0)
		{
			map<string,long long >words(freader(s+"/"+file[i])) ;
			double sum = 0;
			for(map<string,long long >::iterator it = words.begin();it!= words.end();it++)
			{
				string s = (*it).first;
				/*
					tokenization
					normalization
				*/
				insert_trie(s,files.size(),0,(*it).second,head);
				sum+=(*it).second*(*it).second;
			}
			sum = sqrt(sum);
			files.push_back(make_pair(s+"/"+file[i],sum));	
		}
		else
		dreader(s+"/"+file[i]);
	}
}
bool sorter(pair<int,double> a,pair<int,double> b)
{
	return a.second>b.second;
}
//Q is querry after nomalization and spell check increment
vector<string> retire(vector<string>&Q)
{
	vector<string>ans;
	map<int,double >docs;
	for (int i = 0; i < Q.size(); ++i)
	{
		double count = 0;
		vector<pair<int,double> > v(find_trie(Q[i],0,head,&count));
		/*
			add spell correct here
			append words suggestion from dictionary 
			to the back of Q and continue
		*/
		if(v.size() == 0)
		{
			cout<<"The word "<<Q[i]<<" was not found in the dictionary"<<endl;
			continue;
		}
		for (int j = 0; j < v.size(); ++j)
		{
			docs[v[j].first] += v[j].second/(files[v[j].first].second*count);
		}
	}
	vector<pair<int,double> >vec(docs.begin(),docs.end());
	sort(vec.begin(), vec.end(),sorter);
	for(int i=0;i<vec.size();i++)
	{
		ans.push_back(files[vec[i].first].first);
	}
	return ans;
}
void runner()
{
	char c = 'y';
	while(c == 'Y' || c == 'y')
	{
		cout<<"Please enter the Querry"<<endl;
		string chut;
		getline(cin,chut,'.');
		vector<string>Q(toke(chut));
		double d = omp_get_wtime();
		vector<string> ans = retire(Q);
		d = omp_get_wtime() - d;
		cout<<ans.size()<<" results found in "<<d<<" seconds"<<endl;
		if(ans.size() > 0)
		{
			cout<<"Would you like to see the results for your Querry? Y/N"<<endl;
			cin>>c;
			if(c == 'Y' || c == 'y')
			for(int i=0;i<ans.size();i++)
			{
				cout<<ans[i]<<endl;
			}
			cout<<"Would you like to store the results for your Querry? Y/N"<<endl;
			cin>>c;
			if(c == 'Y' || c == 'y')
			{
				cout<<"Enter the name of the file to create/overwrite"<<endl;
				string j;
				cin>>j;
				ofstream out(j.c_str());
				for(int i=0;i<ans.size();i++)
				{
					out<<ans[i]<<endl;
				}	
				out.close();
			}
		}
		cout<<"Would you like to continue with more Querries? Y/N"<<endl;
		cin>>c;
	}
}

int main(int argc, char *argv[])
{
	if(argc == 2)
	{
		string g(argv[1]);
		if(g == "-v");
		{
			verbose = 1;
			cout<<"Verbose mode enabled"<<endl;
		}
	}
	string a = dirchk();
	if(a == "")return 0;
	double d = omp_get_wtime();
	dreader(a);
	d = omp_get_wtime() - d;
	cout<<files.size()<<" files parsed in "<<d<<" seconds"<<endl;
	runner();
	cout<<"Exiting"<<endl;
	return 0;
}/*
int main()
{
	string g = "transPortating, fucked, sucks";
	vector<string> r(toke(g));
	for(auto i:r)
	{
		cout<<i<<endl;
	}
}
*/

