#include <bits/stdc++.h>
#include <fstream>
#include <omp.h>
#include <dirent.h>
#include <tokenizer.hpp>
#include <string>
#include <iostream>
#include "stemming/english_stem.h"
using namespace std;

//verbosity flag
bool verbose = 0;

//trie struct
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

//primary file map vector
vector<pair<string,double> >files;

//primary map pointer
map<char,trie*>head;

//function to test the trie for seg faults
long long recurse1(map<char,trie*>&A)
{
	long long k = 0;
	for(auto i:A)
	{
		k += i.second->doc.size();
		if(i.second->head.size() != 0)k += recurse1(i.second->head); 
	}
	return k;
}

//Edit distance score DP function
int edit_d(string word1, string word2) 
{
	vector<vector<int>> distance(word1.length() + 1, vector<int>(word2.length() + 1, 0));
	for (int i = 0; i < distance.size(); i++) 
	{
	    for (int j = 0; j < distance[0].size(); j++) 
	    {
	        if (i == 0)
	            distance[i][j] = j;
	        else if (j == 0)
	            distance[i][j] = i;
	    }
	}
	for (int i = 1; i < distance.size(); i++) 
	{
	    for (int j = 1; j < distance[0].size(); j++) 
	    {
	        if (word1[i - 1] == word2[j - 1])
	            distance[i][j] = distance[i - 1][j - 1];
	        else
	            distance[i][j] = 1 + min(distance[i - 1][j - 1], min(distance[i - 1][j], distance[i][j - 1]));
	    }
	}

    return distance[word1.length()][word2.length()];    
}

//recursive function for spellcheck from trie
long long recurse2(map<char,trie*>&A,vector<pair<string,int> >&B,string g,string &C)
{
	long long k = 0;
	for(auto i:A)
	{
		k += i.second->doc.size();
		if(i.second->doc.size() != 0)B.push_back(make_pair(g+i.first,edit_d(g+i.first,C)));
		if(i.second->head.size() != 0)k += recurse2(i.second->head,B,g+i.first,C); 
	}
	return k;
}

//function to convert a string to lower case
string to_low(string A)
{
	string g = "";
	for(int i=0;i<A.size();i++)
	{
		g += (A[i]<='Z' && A[i]>='A')?A[i]+'a'-'A':A[i];
	}
	return g;
}

//function to normalize a given string to some root word
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
	std::string st(word.begin(), word.end());
	return st;
}


//tokenizer function
vector<string> toke(string &s)
{
	typedef boost::tokenizer<boost::char_separator<char>> tokenizer;
	boost::char_separator<char> sep{" ,?!;.:=<>(){}[]/-_&+*"};
	tokenizer tok{s, sep};
	vector<string> tokens;
	for (auto &t : tok)
	{
        string st(normalize(to_low(t)));
		if(st!="")
		tokens.push_back(st);
	}
	return tokens;
}

//function to get the files in a directory
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

//function to read and return the map of words in a file
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

//Directory check UI function
string dirchk()
{
	string a;
	cout<<"Enter the name of the directory you would like to pre-process \n";
	cin>>a;
	if(chkdir(a).size() == 0)
	{
		cout<<"Directory "<<a<<" does not exist\n";
		return "";	
	}
	cout<<"Directory "<<a<<" found, now Preprocessing\nPlease note that this might take a while"<<endl;
    return a;
}

//recursive function to insert words to the trie
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

//recursive function to retrieve all the doc_ids from the trie
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

//Recursive function to parse all the files in all the sub-folders
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
//sort criteria functions
bool sorter(pair<int,double> a,pair<int,double> b){return a.second>b.second;}
bool sorted(pair<string,int>a,pair<string,int>b){return a.second < b.second;}

//Q is querry after nomalization and spell check increment
//spell checker function
vector<string> retre(vector<string>&Q)
{
	vector<string>ans;
	map<int,double >docs;
	for (int i = 0; i < Q.size(); ++i)
	{
		double count = 0;
		vector<pair<int,double> > v(find_trie(Q[i],0,head,&count));
		if(v.size() == 0)
		{
			cout<<"The word "<<Q[i]<<" was not found in the dictionary"<<endl;
			cout<<"Do you want to run a spellcheck? Y/N"<<endl;
			char c = 'n';
			cin>>c;
			if(c!= 'y' && c!= 'Y')continue;
			vector<pair<string,int> >B;
			recurse2(head,B,"",Q[i]);
			sort(B.begin(), B.end(),sorted);
			cout<<"please tell which word from the following best suits your purpose?"<<endl;
			for(int i = 0;i<B.size() && i<10;i++)
			{
				cout<<i<<"\t"<<B[i].first<<endl;
			}
			cout<<"Kindly enter the number id of the word"<<endl;
			int y;
			cin>>y;
			v = find_trie(B[y].first,0,head,&count);
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

//function to provide UI to retireve documents
void runner()
{
	cout<<"Please enter the '.' delimited Querry"<<endl;
	string chute;
	getline(cin,chute,'.');
	vector<string>Q(toke(chute));
	vector<string> ans = retre(Q);
	//cout<<ans.size()<<" results found"<<endl;
	if(ans.size() > 0)
	{
		char c;
		cout<<"Would you like to see the results for your Querry? Y/N"<<endl;
		cin>>c;
		if(c == 'Y' || c == 'y')
		for(int i=0;i<ans.size() && i<20;i++)
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
			for(int i=0;i<ans.size() && i<20;i++)
			{
				out<<ans[i]<<endl;
			}	
			out.close();
		}
	}
}

//main function with front end UI all the running codes
void main1(string a="")
{
	while(1)
	{
		cout<<"What would you like to do?\n"
		<<"1.\tRun tokenization and normalization for a query.\n"
		<<"2.\tRun the spell correction for a query.\n"
		<<"3.\tFetch the documents for a query\n"
		<<"4.\tAdd a new directory to the corpus\n"
		<<"Enter your choice"<<endl;
		char c;
		cin>>c;
		if(c == '1')
		{
			string chute;
			cout<<"Please enter the '.' delimited Querry"<<endl;
			getline(cin,chute,'.');		
			vector<string>Q(toke(chute));
			for(int i=0;i<Q.size();i++)
			{
				cout<<Q[i]<<"\t";
			}
			cout<<endl;
		}
		else if(c == '2')
		{
			cout<<"Enter the word you would like to run spellcheck on"<<endl;
			string g;
			cin>>g;
			cin.clear();
			double count;
			vector<pair<int,double> >Q(find_trie(g,0,head,&count));
			if(Q.size()!=0)
				cout<<"Spellcheck found that word exists in dictionary and is therefore correct.\n";
			else
			{
				cout<<"SpellCheck did not find the word in dictionary\n"
				<<"Running SpellCorrect"<<endl;
				vector<pair<string,int> >B;
				recurse2(head,B,"",g);
				sort(B.begin(), B.end(),sorted);
				cout<<"The Following words are found to be the nearest match for your word"<<endl;
				for(int i = 0;i<B.size() && i<20;i++)
				{
					cout<<i+1<<"\t"<<B[i].first<<endl;
				}
			}
		}
		else if(c == '3')
		{
			runner();
		}
		else if(c == '4')
		{
			string a = dirchk();
			if(a == "")continue;
			double d = omp_get_wtime();
			dreader(a);
			d = omp_get_wtime() - d;
			if(verbose)cout<<recurse1(head)<<" distinct words of "
			<<files.size()<<" files parsed in "<<d<<" seconds"<<endl;
		}
		else
		{
			cout<<"You entered wrong choice!"<<endl;
		}
		cout<<"Would you like to continue? Y/N"<<endl;
		c = 'n';
		cin>>c;
		if(c!='y' && c!= 'C')break;
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
	main1();
	cout<<"Thank You!!\nExiting"<<endl;
	return 0;
}
