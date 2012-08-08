class Administrators::LookingForsController < ApplicationController

  def index
    if params[:search]
      @looking_fors = LookingFor.search_properties(params[:search]).paginate(:page => params[:page], :per_page => params[:per_page] , :order =>"created_at  DESC")
    else
      @looking_fors = LookingFor.paginate(:page => params[:page], :per_page => params[:per_page] , :order =>"created_at  DESC")
    end
  end

  def new
    @looking_for = LookingFor.new
  end

  def create
    @looking_for = LookingFor.new(params[:looking_for])
    #@looking_for.user_id = current_user.id
    respond_to do |format|
      if  @looking_for.save
        format.html { redirect_to ([:administrators,@looking_for]) ,:notice => "Property Saved Successfully." }
      else
        format.html { render :action => "new" }
        format.xml  { render :xml => @looking_for.errors, :status => :unprocessable_entity }
      end
    end
  end

  def edit
    @looking_for = LookingFor.find(params[:id])
  end

  def update
    @looking_for = LookingFor.find(params[:id])
    #@looking_for.user_id = current_user.id
    if  @looking_for.update_attributes(params[:looking_for])
      redirect_to([:administrators,@looking_for])
      flash[:notice] = "Property Updated Successfully."
    else
      render :action => "edit"
    end
  end

  def show
    @looking_for = LookingFor.find(params[:id])
  end

  def destroy
    @looking_for = LookingFor.find(params[:id])
    @looking_for.destroy
    redirect_to administrators_looking_fors_path
  end

end
